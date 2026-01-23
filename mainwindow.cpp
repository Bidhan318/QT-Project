#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "login_window.h"
#include <QHostAddress>
#include "port.h"
#include <QSystemTrayIcon>
#include <QIcon>
#include <QTimer>
#include <QString>
#include <QNetworkInterface>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QDateTime>
#include <QCryptographicHash>

MainWindow::MainWindow(QString username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , LoggedUser(username)
    , hasattachedFile(false)
    , isSendingFile(false)
    , isLoggingOut(false)
{
    ui->setupUi(this);

    setemojiBtn(); //configure the emojis

    //----UDP for presence detction and TCP for msg passing----------------

    // UDP socket setup for presence detection
    udpSocket = new QUdpSocket(this);  //using this makes mainwindow parent so socket closes when the window is closed
    udpSocket->bind(QHostAddress::AnyIPv4, PORT,
                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    // TCP socket setup for msgs
    tcpSocket = new QTcpSocket(this);

    // Set up periodic presence announcement (every 3 seconds)
    announcementTimer = new QTimer(this);
    connect(announcementTimer, &QTimer::timeout, this, &MainWindow::announcePresence);
    announcementTimer->start(2000); // Announce every 2 seconds

    // Send initial announcement immediately
    QTimer::singleShot(500, this, &MainWindow::announcePresence); // Delay 500ms to ensure socket is ready

    // System tray icon (required for notifications)
    trayicon = new QSystemTrayIcon(this);
    trayicon->setIcon(QIcon(":/images/novachat.png"));
    trayicon->setToolTip("NovaChat");
    trayicon->show();

    //Create default "ALL tab chat"
    QTextEdit *allview = new QTextEdit;
    allview->setReadOnly(true);
    allview->setPlaceholderText("Broadcast messages appear here...");
    ui->chatTabs->addTab(allview,"All");
    chatTabs.insert("All",allview);  //add in qmap

    //initialize active list
    if(ui->activelist)
    {
        ui->activelist->addItem("All");
        ui->activelist->setCurrentIndex(0);
    }

    // UI connections
    connect(ui->sendBtn, &QPushButton::clicked,
            this, &MainWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed,
            this, &MainWindow::sendMessage);      //for working when pressed enter
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &MainWindow::receivePresenceAnnouncement);  //signal when udp packet arrives

    // TCP connections
    connect(tcpSocket, &QTcpSocket::connected,  //signal when tcp connection establishes
            this, &MainWindow::onConnectedToServer);
    connect(tcpSocket, &QTcpSocket::disconnected,
            this, &MainWindow::onDisconnectedFromServer);
    connect(tcpSocket, &QTcpSocket::errorOccurred,
            this, &MainWindow::onTcpError);
    connect(tcpSocket,&QTcpSocket::readyRead,
            this, &MainWindow::receiveMessage);

    // When user clicks a tab → update active list
    connect(ui->chatTabs, &QTabWidget::currentChanged,
            this, &MainWindow::onTabChanged);

    // When user selects from active list → change tab
    connect(ui->activelist, &QComboBox::currentTextChanged,
            this, &MainWindow::onActivelistChanged);


    // Connect to server
    connectToServer();


    // HIDE FILE TRANSFER WIDGETS INITIALLY (just add these lines!)
    if (ui->fileProgressBar) {
        ui->fileProgressBar->setVisible(false);
        ui->fileProgressBar->setValue(0);
    }

    if (ui->fileStatusLabel) {
        ui->fileStatusLabel->setVisible(false);
        ui->fileStatusLabel->setText("");
    }

    if (ui->cancelFileBtn) {
        ui->cancelFileBtn->setVisible(false);
    }

    loadChatHistoryForTab("All"); // Load old messages into all tab initially

    connect(ui->clearChatBtn_, &QPushButton::clicked,
            this, &MainWindow::clearCurrentChatHistory);
}

MainWindow::~MainWindow()
{
    if(announcementTimer)
    {
        announcementTimer->stop();
    }
    announceDeparture();

    for(auto it = activeDownloads.begin(); it != activeDownloads.end(); ++it)
    {
        IncomingFile &incoming = it.value();
        if(incoming.file)
        {
            incoming.file->close();
            delete incoming.file;
        }
    }
    activeDownloads.clear();

    // Notify server of disconnect
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
        tcpSocket->waitForDisconnected(1000); //waits 1000ms = 1sec for disconnect to complete
    }
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)  //for when window directly closed so the activelist updates
{
    if(isLoggingOut) //no manual cleanup already handled
    {
        event->accept();
        return;
    }

    // Stop presence announcements
    if (announcementTimer)
        announcementTimer->stop();

    // Notify other clients (UDP)
    announceDeparture();
    QThread::msleep(200);//give some time for udp packets to leave

    // Just disconnect TCP (server will handle cleanup)
    if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
        tcpSocket->waitForDisconnected(500);
    }

    event->accept();  // allow window to close
}

void MainWindow::connectToServer()
{
    // // Only connect if not already connected or connecting
    // if (tcpSocket->state() == QAbstractSocket::UnconnectedState)
    // {
    //     // Try connecting to hardcoded IP first
    //     // CHANGE THIS to your server's actual IP address (find it using ipconfig on server laptop)
    //     QString serverIP = "192.168.1.100";

    //     tcpSocket->connectToHost(QHostAddress(serverIP), TCP_PORT);

    //     // If this fails, SERVER_DISCOVERY will retry with correct IP as backup
    // }
}

void MainWindow::onConnectedToServer()  //auto called when tcp connection succeeds checked in serverwindow::onClientDataReceived
{
    // Send username to server for registration
    QString loginMsg = "LOGIN:" + LoggedUser + "\n";
    tcpSocket->write(loginMsg.toUtf8());  //converts qstring to byte array for sending
    tcpSocket->flush();  //force immediate sending of data
}

void MainWindow::onDisconnectedFromServer() //for when cross pressed by server in tab list
{

    if (chatTabs.contains("All")) {
        chatTabs["All"]->append("--- Disconnected from server ---");
    }
    ui->messageEdit->setEnabled(false);
    ui->sendBtn->setEnabled(false);
    if(ui->activelist)
    {
        ui->activelist->setEnabled(false);
    }
}

void MainWindow::onTcpError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    // Silent error - no popup
}

void MainWindow::sendMessage()
{
    QString msg = ui->messageEdit->text();
    QString destination = ui->activelist->currentText(); //accesses the text of drop down active list
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm A");

    //check if connected to server
    if(tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        if(chatTabs.contains("All"))
        {
            chatTabs["All"]->append("---Not connected to server---");
        }
        return;
    }
    // ONLY block if actively transferring (after approval)
    if(isSendingFile)
    {
        QMessageBox::warning(this, "File Transfer",
                             "File transfer in progress. Please wait.");
        return;
    }

    //  HANDLE FILE SENDING
    if(hasattachedFile && !pendingFilePath.isEmpty())
    {
        if(destination == "All")
        {
            QMessageBox::warning(this, "File Transfer",
                                 "Cannot send files to 'All'. Please select a specific user.");
            return;
        }

        sendFile(destination,msg); //msg is the optional caption

        // clear the UI text
        ui->messageEdit->clear();
        ui->messageEdit->setPlaceholderText("Type a message...");
        return;
    }

    if (msg.isEmpty()) return;


    if(destination=="All")
    {
        // Send broadcast message to everyone
        QString fullMsg = "BROADCAST:" + LoggedUser + ":" + msg +"\n";
        tcpSocket->write(fullMsg.toUtf8());
        tcpSocket->flush();

        //show in all tab
        if(chatTabs.contains("All"))
        {
            appendAlignedMessage(
                chatTabs["All"],
                "[" + timestamp + "] Me: " + msg,
                Qt::AlignRight
                );
        }
    }
    else{
        //send private messages
        QString privateMsg = "PRIVATE:" + destination + ":" + LoggedUser + ":" + msg + "\n";
        tcpSocket->write(privateMsg.toUtf8());
        tcpSocket->flush();

        if(chatTabs.contains(destination))
        {
            appendAlignedMessage(
                chatTabs[destination],
                "[" + timestamp + "] Me: " + msg,
                Qt::AlignRight
                );
        }
    }
    ui->messageEdit->clear();
    saveChatHistory();  // Save after sending
}

void MainWindow::receiveMessage()
{
    //read all available data from tcp socket
    QByteArray data = tcpSocket->readAll();

    // DEBUG: Log what we received
    qDebug() << "Received" << data.size() << "bytes";

    //PRIORITY 1: check if we are recieving a file (already in download mode)
    if (!activeDownloads.isEmpty())
    {
        for(auto it = activeDownloads.begin(); it != activeDownloads.end(); ++it)
        {
            QString transferId = it.key();
            IncomingFile &incoming = it.value();

            // Work with raw bytes - NO QString conversion
            int endMarkerPos = data.indexOf("FILE_TRANSFER_COMPLETE:");

            if(endMarkerPos != -1)
            {
                QByteArray fileData = data.left(endMarkerPos);
                if(!fileData.isEmpty() && incoming.file)
                {
                    incoming.file->write(fileData);
                    incoming.bytesReceived += fileData.size();
                    incoming.file->flush();
                }

                handleFileComplete(transferId);
                return;  // Exit early
            }

            // Regular file chunk - write raw bytes
            if(incoming.file)
            {
                qint64 written = incoming.file->write(data);  // Write raw bytes!
                incoming.bytesReceived += written;
                incoming.file->flush();

                // Update UI
                if (incoming.totalSize > 0)
                {
                    int progress = (incoming.bytesReceived * 100) / incoming.totalSize;

                    if (ui->fileProgressBar) {
                        ui->fileProgressBar->setValue(progress);
                    }

                    if (ui->fileStatusLabel) {
                        ui->fileStatusLabel->setText(
                            QString("Receiving %1... %2% (%3/%4 MB)")
                                .arg(incoming.fileName)
                                .arg(progress)
                                .arg(incoming.bytesReceived / 1024.0 / 1024.0, 0, 'f', 2)
                                .arg(incoming.totalSize / 1024.0 / 1024.0, 0, 'f', 2)
                            );
                    }

                    QCoreApplication::processEvents();  // Force UI update
                }
            }

            return;  // Exit - don't process as text
        }
    }

    //PRIORITY 2: check if FILE_TRANSFER_START is in this packet (before converting to string)
    //this handles race condition where START message + file data arrive in same TCP packet
    int startPos = data.indexOf("FILE_TRANSFER_START:");
    if (startPos != -1)
    {
        qDebug() << "Found FILE_TRANSFER_START message";  // DEBUG
        int endOfLine = data.indexOf('\n', startPos);
        if (endOfLine != -1)
        {
            //extract ONLY the control message part (not the binary data after it)
            QByteArray controlMsg = data.mid(startPos, endOfLine - startPos);
            QString msg = QString::fromUtf8(controlMsg);

            //parse the START message
            QStringList parts = msg.split(':');
            if(parts.size() >= 5)
            {
                QString transferId = parts[1];
                QString sender = parts[2];
                QString fileName = parts[3];
                qint64 fileSize = parts[4].toLongLong();
                QString caption = parts.size() > 5 ? parts.mid(5).join(':') : "";

                //setup the download (creates file, adds to activeDownloads)
                handleFileTransferStart(transferId, sender, fileName, fileSize, caption);

                //check if file data came right after the START message in same packet
                int fileDataStart = endOfLine + 1;
                if (fileDataStart < data.size() && !activeDownloads.isEmpty())
                {
                    //extract the binary file data that arrived with the START message
                    QByteArray fileData = data.mid(fileDataStart);

                    //write this first chunk immediatly
                    auto it = activeDownloads.begin();
                    IncomingFile &incoming = it.value();
                    if (incoming.file && !fileData.isEmpty())
                    {
                        incoming.file->write(fileData);
                        incoming.bytesReceived += fileData.size();
                        incoming.file->flush();

                        //update progress bar with first chunk
                        if (incoming.totalSize > 0)
                        {
                            int progress = (incoming.bytesReceived * 100) / incoming.totalSize;
                            if (ui->fileProgressBar) {
                                ui->fileProgressBar->setValue(progress);
                            }
                            if (ui->fileStatusLabel) {
                                ui->fileStatusLabel->setText(
                                    QString("Receiving %1... %2%")
                                        .arg(incoming.fileName)
                                        .arg(progress)
                                    );
                            }
                        }
                    }
                }
            }

            return;  //exit - we've handled this packet completely
        }
    }

    //PRIORITY 3: normal text message processing (no binary data in this packet)
    QString allData = QString::fromUtf8(data);
    //split by newlines in case multiple msgs arrived together
    QStringList messages = allData.split('\n', Qt::SkipEmptyParts);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm A");

    for(const QString &msg : messages)
    {
        if(msg.isEmpty()) continue;


        //handle file transfer blocked
        if(msg.startsWith("FILE_TRANSFER_BLOCKED:"))
        {
            QString reason = msg.mid(22).trimmed();
            handleFileTransferBlocked(reason);
            // Clean up attachment state
            pendingFilePath.clear();
            hasattachedFile = false;
            isSendingFile = false;
            currentTransferId.clear();

            // Reset attach button
            if (ui->attachFile) {
                ui->attachFile->setText("📎");
                ui->attachFile->setToolTip("Attach file");
            }

            // Reset message input
            ui->messageEdit->setPlaceholderText("Type a message...");

            // Hide progress UI
            if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
            if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
            if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);
            continue;
        }

        // Handle FILE_TRANSFER_WAITING (sender side - just show status)
        if (msg.startsWith("FILE_TRANSFER_WAITING:"))
        {
            QString transferId = msg.mid(22).trimmed();
            isSendingFile = false; //just waiting reset the flag here

            if (ui->fileStatusLabel) {
                ui->fileStatusLabel->setVisible(true);
                ui->fileStatusLabel->setText("Waiting for recipient to accept...");
            }

            continue;
        }

        // Handle FILE_TRANSFER_PENDING (recipient side - show approval dialog)
        if (msg.startsWith("FILE_TRANSFER_PENDING:"))
        {
            QStringList parts = msg.split(':');
            isSendingFile = false;
            if (parts.size() >= 5)
            {
                QString transferId = parts[1];
                QString sender = parts[2];
                QString fileName = parts[3];
                qint64 fileSize = parts[4].toLongLong();
                QString caption = parts.size() > 5 ? parts.mid(5).join(':') : "";

                // Store transfer ID for response
                pendingTransferId = transferId;

                // Build approval message
                QString approvalMsg = QString("%1 wants to send you:\n\n📎 %2 (%3 MB)")
                                          .arg(sender)
                                          .arg(fileName)
                                          .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2);

                if (!caption.isEmpty()) {
                    approvalMsg += QString("\n\n💬 Caption: %1").arg(caption);
                }

                approvalMsg += "\n\nDo you want to accept this file?";

                // Show NON-BLOCKING dialog with custom buttons
                QMessageBox *msgBox = new QMessageBox(this);
                msgBox->setWindowTitle("Incoming File Transfer");
                msgBox->setText(approvalMsg);
                msgBox->setIcon(QMessageBox::Question);

                QPushButton *acceptBtn = msgBox->addButton("Accept", QMessageBox::AcceptRole);
                QPushButton *rejectBtn = msgBox->addButton("Decline", QMessageBox::RejectRole);

                msgBox->setDefaultButton(acceptBtn);

                // Connect to slots for handling the response
                connect(msgBox, &QMessageBox::finished, this, [this, msgBox, acceptBtn, transferId, sender]() {
                    if (msgBox->clickedButton() == acceptBtn) {
                        // User accepted
                        QString response = QString("FILE_TRANSFER_ACCEPTED:%1\n").arg(transferId);
                        tcpSocket->write(response.toUtf8());
                        tcpSocket->flush();

                        // Show preparing message
                        if (ui->fileStatusLabel) {
                            ui->fileStatusLabel->setVisible(true);
                            ui->fileStatusLabel->setText("Preparing to receive file...");
                        }
                    } else {
                        // User declined
                        QString response = QString("FILE_TRANSFER_REJECTED:%1\n").arg(transferId);
                        tcpSocket->write(response.toUtf8());
                        tcpSocket->flush();

                        // Show notification in chat
                        if (chatTabs.contains(sender)) {
                            chatTabs[sender]->append("--- You declined the file transfer ---");
                        }
                    }

                    pendingTransferId.clear();
                    msgBox->deleteLater();
                });

                // Show dialog (non-blocking!)
                msgBox->show();
            }
            continue;
        }

        //handle file transfer approved
        if (msg.startsWith("FILE_TRANSFER_APPROVED:"))
        {
            QString transferId = msg.mid(23).trimmed();
            currentTransferId = transferId;

            isSendingFile = true;
            //send the file data
            QFile file(pendingFilePath);
            if(!file.open(QIODevice::ReadOnly))
            {
                QString errorMsg = QString("Cannot open file for reading: %1\nPath: %2")
                .arg(file.errorString())
                    .arg(pendingFilePath);
                QMessageBox::critical(this, "Error", errorMsg);
                isSendingFile = false;

                // Clean up UI
                if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
                if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
                if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);

                return;
            }

            QFileInfo fileInfo(pendingFilePath);
            qint64 fileSize = fileInfo.size();

            //show progress ui
            if (ui->fileProgressBar) {
                ui->fileProgressBar->setVisible(true);
                ui->fileProgressBar->setValue(0);
                ui->fileProgressBar->setMaximum(100);
            }

            if (ui->fileStatusLabel) {
                ui->fileStatusLabel->setVisible(true);
                ui->fileStatusLabel->setText("Sending " + fileInfo.fileName() + "...");
            }

            if (ui->cancelFileBtn) {
                ui->cancelFileBtn->setVisible(true);
            }

            //send file data in chunks
            const qint64 chunkSize = 64 * 1024; //64 kb
            qint64 bytesSent = 0;

            while(!file.atEnd())
            {
                QByteArray chunk = file.read(chunkSize);
                if (chunk.isEmpty()) break;

                qint64 written = tcpSocket->write(chunk);

                if(written == -1)
                {
                    QMessageBox::critical(this, "Error", "Failed to send file data.");
                    file.close();
                    isSendingFile = false;

                    if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
                    if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
                    if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);

                    return;
                }
                bytesSent += written;

                // IMPORTANT: Wait for data to be written before continuing
                if (!tcpSocket->waitForBytesWritten(3000)) {
                    QMessageBox::critical(this, "Error", "Timeout sending file data.");
                    file.close();
                    isSendingFile = false;

                    if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
                    if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
                    if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);

                    return;
                }

                //update progress bar
                int progress = (bytesSent * 100) / fileSize;
                if (ui->fileProgressBar) {
                    ui->fileProgressBar->setValue(progress);
                }
                // Update status with progress
                if (ui->fileStatusLabel) {
                    ui->fileStatusLabel->setText(QString("Sending %1... %2%")
                                                     .arg(fileInfo.fileName())
                                                     .arg(progress));
                }

                QCoreApplication::processEvents();
            }
            file.close();
            QThread::msleep(100);
            //send completion msg
            QString completion = QString("FILE_TRANSFER_COMPLETE:%1\n").arg(transferId);
            tcpSocket->write(completion.toUtf8());
            tcpSocket->flush();

            //update ui
            QString recipient =currentFilerecipient;
            if (chatTabs.contains(recipient))
            {
                QString displayMsg = QString("[%1] Me: 📎 Sent file: %2 (%3 MB)")
                                         .arg(timestamp)
                                         .arg(fileInfo.fileName())
                                         .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2);

                appendAlignedMessage(chatTabs[recipient], displayMsg, Qt::AlignRight);
            }

            //clean up
            isSendingFile = false;
            currentTransferId.clear();

            // NOW clear the attachment state (after file is sent)
            pendingFilePath.clear();
            hasattachedFile = false;

            if (ui->attachFile) {
                ui->attachFile->setText("📎");
                ui->attachFile->setToolTip("Attach file");
            }

            QTimer::singleShot(1000, this, [this]() {
                if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
                if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
                if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);
            });

            saveChatHistory();
            continue;
        }

        //NOTE: FILE_TRANSFER_START is now handled above (PRIORITY 2) before string conversion
        //removed from here to prevent binary data corruption

        //handle file transfer cancled
        if (msg.startsWith("FILE_TRANSFER_CANCELLED:"))
        {
            QString transferId = msg.mid(24).trimmed();
            handleFileCancelled(transferId);
            continue;
        }

        //handle server shutdown msg
        if (msg == "SERVER_SHUTDOWN")
        {
            // STOP UDP announcements immediately
            if(announcementTimer)
            {
                announcementTimer->stop();
            }
            // Close UDP socket to stop receiving discovery messages
            if(udpSocket)
            {
                udpSocket->close();
            }
            for (QTextEdit *view : chatTabs.values()) {
                if (view) {
                    view->append("--- Server has disconnected ---");
                    view->setEnabled(false);
                }
            }
            ui->messageEdit->setEnabled(false);
            ui->sendBtn->setEnabled(false);

            if(ui->activelist)
            {
                ui->activelist->setEnabled(false);
            }

            if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                tcpSocket->disconnectFromHost();
            }

            continue;
        }

        //handle kicked msg
        if(msg == "KICKED")
        {
            isLoggingOut = true;
            // STOP announcing presence immediately
            if(announcementTimer)
            {
                announcementTimer->stop();
            }
            announceDeparture();

            // Close UDP socket to stop receiving discovery messages
            if(udpSocket)
            {
                udpSocket->close();
            }

            if(chatTabs.contains("All"))
            {
                chatTabs["All"]->append("---You have been disconnected by the server---");
            }

            ui->messageEdit->setEnabled(false);
            ui->sendBtn->setEnabled(false);
            if(ui->activelist)
            {
                ui->activelist->setEnabled(false);
            }
            if(tcpSocket->state() == QAbstractSocket::ConnectedState)
            {
                tcpSocket->disconnectFromHost();
            }

            // Show message box to inform user
            QMessageBox::information(this, "Kicked", "You have been disconnected by the server.");

            // Open login window and destroy this window
            loginwindow = new Login_Window();
            loginwindow->show();
            deleteLater();  // Completely destroy MainWindow
            return;
        }


        //parse msg format TYPE:sender:msg or Type:recipient:sender:msg
        QStringList parts = msg.split(':'); //just keeps the part of msg seperate from each of :

        // Handle SERVER messages first (they only have 2 parts)
        if(parts.size() >= 2 && parts[0] == "SERVER")
        {
            QString message = parts.mid(1).join(':');

            if(chatTabs.contains("All"))
            {
                appendAlignedMessage(
                    chatTabs["All"],
                    "[" + timestamp + "] Server: " + message,
                    Qt::AlignLeft
                    );

                saveChatHistory();
            }
            if (windowState() & Qt::WindowMinimized || !isActiveWindow())
            {
                trayicon->showMessage(
                    "Server message — NovaChat",
                    "Server: " + message,
                    QSystemTrayIcon::Information,
                    3000
                    );
            }
            continue;
        }

        if(parts.size() < 3) continue; //has to be >=3 due to our format

        QString msgtype = parts[0]; //extract the "TYPE" part from the entire recieved msg

        if(msgtype == "BROADCAST")
        {
            QString sender = parts[1];
            QString message = parts.mid(2).join(':'); //rejoin if the msg itself has ":"

            //dont show your own msg
            if(sender == LoggedUser) continue;

            if(chatTabs.contains("All"))
            {
                appendAlignedMessage(
                    chatTabs["All"],
                    "[" + timestamp + "] " + sender + ": " + message,
                    Qt::AlignLeft
                    );

                saveChatHistory();
            }

            //show noti
            if(windowState() & Qt::WindowMinimized || !isActiveWindow())
            {
                trayicon->showMessage(
                    "New message - Novachat",
                    sender + ": " + message,
                    QSystemTrayIcon::Information,
                    3000);
            }
        }

        else if (msgtype == "PRIVATE")
        {
            QString sender = parts[1];
            QString message = parts.mid(2).join(':');  //modified by server not the exact format as in the mainwindow::sendmsg

            //ensure tab exists for pvt msgs
            if(!chatTabs.contains(sender))
            {
                addChatTab(sender);
                if(ui->activelist)
                {
                    ui->activelist->addItem(sender);
                }
                if(!activeClients.contains(sender))
                {
                    activeClients.insert(sender,QHostAddress()); //update qmap
                }
                loadChatHistoryForTab(sender);
            }
            //display pvt msgs in senders tab
            appendAlignedMessage(
                chatTabs[sender],
                "[" + timestamp + "] " + sender + ": " + message,
                Qt::AlignLeft
                );

            saveChatHistory();

            //show noti
            if (windowState() & Qt::WindowMinimized || !isActiveWindow())
            {
                trayicon->showMessage(
                    "New private message — NovaChat",
                    sender + ": " + message,
                    QSystemTrayIcon::Information,
                    3000
                    );
            }
        }
    }
}

void MainWindow::receivePresenceAnnouncement()
{
    //handle udp announcements
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress senderIP;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(),
                                &senderIP, &senderPort);
        QString msg = QString::fromUtf8(datagram);

        //check for server discovery msg
        if(msg.startsWith("SERVER_DISCOVERY"))
        {
            //extract the server ip
            QString serverIP = "127.0.0.1";
            if(msg.contains(":"))
            {
                serverIP = msg.mid(msg.indexOf(':')+1).trimmed();
            }

            //connect to server via TCP if not connected
            if(tcpSocket->state() != QAbstractSocket::ConnectedState)
            {
                if(tcpSocket->state() != QAbstractSocket::UnconnectedState)
                {
                    tcpSocket->abort(); //aborts and resets connection
                }
                tcpSocket->connectToHost(QHostAddress(serverIP), TCP_PORT);
                //connects the clients to server
            }
            continue;
        }

        //handle client announce msgs
        if(msg.startsWith("CLIENT_ANNOUNCE:"))
        {
            QString announcedUser = msg.mid(16).trimmed(); //trims the username to the right of ":"
            //dont add yourself

            if(announcedUser != LoggedUser && announcedUser != "Server")
            {
                //update IP address
                activeClients[announcedUser] = senderIP;

                //only add ui for a new user
                if(!chatTabs.contains(announcedUser))
                {
                    if(ui->activelist)
                    {
                        ui->activelist->addItem(announcedUser);
                    }
                    addChatTab(announcedUser);

                    loadChatHistoryForTab(announcedUser);  // Load history when user comes online
                }
            }
            continue;
        }

        if(msg.startsWith("CLIENT_DEPARTURE:"))
        {
            QString departedUser = msg.mid(17).trimmed();
            if(departedUser != LoggedUser && departedUser != "Server")
            {
                // Remove from active clients qmap
                activeClients.remove(departedUser);

                // Remove from active list dropdown
                if(ui->activelist)
                {
                    int index = ui->activelist->findText(departedUser);
                    if(index != -1)
                    {
                        ui->activelist->removeItem(index);
                    }
                }

                // Remove chat tab
                removeChatTab(departedUser);
            }
            continue;
        }
    }
}

//to add and remove tabs for messaging

void MainWindow::addChatTab(const QString &username)
{
    if(chatTabs.contains(username)) return; //check if tab exists
    QTextEdit *view = new QTextEdit;
    view->setReadOnly(true);
    view->setPlaceholderText("Private chats with "+ username + " here...");

    //add the tab
    ui->chatTabs->addTab(view,username);
    chatTabs.insert(username,view);  //add in qmap
}

void MainWindow::removeChatTab(const QString &username)
{
    if(username == "All") return;
    if(!chatTabs.contains(username))  return;
    QTextEdit *view = chatTabs.value(username);  //access the specific user's tab

    //safety check
    if(!view)
    {
        chatTabs.remove(username);
        return;
    }
    int index = ui->chatTabs->indexOf(view);
    if(index !=-1)
    {
        ui->chatTabs->removeTab(index);
    }
    chatTabs.remove(username);  //remove from qmap
    delete view;
}

QTextEdit* MainWindow::getCurrentChatView()
{
    QWidget *currentwidget = ui->chatTabs->currentWidget();
    return qobject_cast<QTextEdit*>(currentwidget);
}

void MainWindow::announcePresence()
{
    // Broadcast presence to all clients so they know we're online
    QString announce = "CLIENT_ANNOUNCE:"+LoggedUser;
    for (int i = 0; i < 5; i++) {
        udpSocket->writeDatagram(announce.toUtf8(), QHostAddress::Broadcast, PORT);
        QCoreApplication::processEvents();  // Process any incoming packets
        QThread::msleep(30);  // Shorter delay between sends
    }
}

void MainWindow::announceDeparture()
{
    //broadcast departure to all clients
    QString departure = "CLIENT_DEPARTURE:" + LoggedUser;

    for (int i = 0; i < 5; i++) {
        udpSocket->writeDatagram(departure.toUtf8(), QHostAddress::Broadcast, PORT);
        QCoreApplication::processEvents();  // processes all other pending udp packets
        QThread::msleep(100);
    }
}

//refine ui funcs

void MainWindow::onTabChanged(int index)
{
    if(index< 0) return;
    QString tabname = ui->chatTabs->tabText(index);

    if(ui->activelist->currentText() != tabname)
    {
        int comboindex = ui->activelist->findText(tabname);
        if (comboindex != -1)
        {
            ui->activelist->setCurrentIndex(comboindex);
        }
    }
}


void MainWindow::onActivelistChanged(const QString& username)
{
    if(!chatTabs.contains(username)) return;
    QTextEdit *view = chatTabs.value(username);
    int tabindex = ui->chatTabs->indexOf(view);
    if(tabindex != -1 && ui->chatTabs->currentIndex() != tabindex)
    {
        ui->chatTabs->setCurrentIndex(tabindex);
    }
}

void MainWindow::on_logout_clicked()
{
    isLoggingOut = true; //so that the overide func doesnt occur
    if(announcementTimer)
    {
        announcementTimer->stop();
    }
    announceDeparture();

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) //on logout notify via tcp
    {
        QString logoutMsg = "LOGOUT:" + LoggedUser + "\n";
        tcpSocket->write(logoutMsg.toUtf8());
        tcpSocket->flush();
        tcpSocket->waitForBytesWritten(1000);

        tcpSocket->disconnectFromHost();
        tcpSocket->waitForDisconnected(1000);
    }

    loginwindow = new Login_Window();
    loginwindow->show();

    deleteLater();

}

// ===== SIMPLE REVERSIBLE ENCRYPTION (LOCAL STORAGE ONLY) =====
QByteArray MainWindow::getKey()
{
    return QCryptographicHash::hash(
        QString("NovaChat-Local-Key").toUtf8(),
        QCryptographicHash::Sha256
        );
}

QString MainWindow::encryptString(const QString &plain)
{
    QByteArray data = plain.toUtf8();
    QByteArray key = getKey();

    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];

    return QString::fromUtf8(data.toBase64());
}

QString MainWindow::decryptString(const QString &encrypted)
{
    QByteArray data = QByteArray::fromBase64(encrypted.toUtf8());
    QByteArray key = getKey();

    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];

    return QString::fromUtf8(data);
}



void MainWindow::loadChatHistory()  //load for all tab
{
    QString historyFile = QCoreApplication::applicationDirPath() + "/chat_history.json";
    QFile file(historyFile);
    if (!file.exists()) return;  // No history yet

    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    // Get users object
    if (!root.contains("users")) return;
    QJsonObject users = root["users"].toObject();

    // Get THIS user's data
    if (!users.contains(LoggedUser)) return;  // No history for this user
    QJsonObject userData = users[LoggedUser].toObject();

    //Only load history for tabs that ALREADY EXIST
    for (QString tabName : userData.keys())
    {
        // Only load if tab already exists (user is online or it's "All")
        if (!chatTabs.contains(tabName)) {
            continue;  // Skip - don't create tab, user is offline
        }

        QTextEdit *view = chatTabs.value(tabName);
        if (!view) continue;

        QJsonArray messages = userData[tabName].toArray();

        for (const QJsonValue &msgVal : messages)
        {
            QJsonObject msgObj = msgVal.toObject();
            QString timestamp = decryptString(msgObj["timestamp"].toString());
            QString from = decryptString(msgObj["from"].toString());
            QString message = decryptString(msgObj["message"].toString());

            Qt::Alignment align = (from == LoggedUser) ? Qt::AlignRight : Qt::AlignLeft;
            QString displayFrom = (from == LoggedUser) ? "Me" : from;

            appendAlignedMessage(
                view,
                "[" + timestamp + "] " + displayFrom + ": " + message,
                align
                );
        }
    }
}

void MainWindow::saveChatHistory()
{
    QString historyFile = QCoreApplication::applicationDirPath() + "/chat_history.json";
    QFile file(historyFile);

    // Load existing data first
    QJsonObject root;
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                root = doc.object();
            }
            file.close();
        }
    }

    // Ensure structure exists
    if (!root.contains("users")) {
        root["users"] = QJsonObject();
    }

    // Get users object
    QJsonObject users = root["users"].toObject();

    // Get THIS user's data (or create new)
    QJsonObject userData = users.contains(LoggedUser)
                               ? users[LoggedUser].toObject()
                               : QJsonObject();

    for (QString tabName : chatTabs.keys())
    {
        QTextEdit *view = chatTabs[tabName];
        if (!view) continue;

        QJsonArray messages;
        QStringList lines = view->toPlainText().split("\n");

        for (const QString &line : lines)
        {
            if (line.isEmpty()) continue;  // Skip empty lines

            // Skip system messages (lines with ---)
            if (line.contains("---")) continue;

            //  "[02:30 PM] username: message"
            if (!line.startsWith("[")) continue;

            int timestampEnd = line.indexOf("]");
            if (timestampEnd == -1) continue;

            QString timestamp = line.mid(1, timestampEnd - 1).trimmed();
            QString rest = line.mid(timestampEnd + 1).trimmed();

            int sep = rest.indexOf(":"); // Split "from: message"
            if (sep == -1) continue;

            QString from = rest.left(sep).trimmed();
            if (from == "Me") from = LoggedUser;

            QString message = rest.mid(sep + 1).trimmed();

            QJsonObject msgObj;
            msgObj["timestamp"] = encryptString(timestamp);
            msgObj["from"] = encryptString(from);
            msgObj["message"] = encryptString(message);

            messages.append(msgObj);
        }

        userData[tabName] = messages;  // Save to THIS user's data
    }

    users[LoggedUser] = userData;  // Update THIS user's section
    root["users"] = users;

    // Write back to file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}


void MainWindow::loadChatHistoryForTab(const QString &tabName) //load for pvt msgs
{
    QString historyFile = QCoreApplication::applicationDirPath() + "/chat_history.json";
    QFile file(historyFile);
    if (!file.exists()) return;

    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    if (!root.contains("users")) return;

    QJsonObject users = root["users"].toObject();
    if (!users.contains(LoggedUser)) return;

    QJsonObject userData = users[LoggedUser].toObject();
    if (!userData.contains(tabName)) return;  // No history for this user

    QTextEdit *view = chatTabs.value(tabName);
    if (!view) return;

    QJsonArray messages = userData[tabName].toArray();

    for (const QJsonValue &msgVal : messages)
    {
        QJsonObject msgObj = msgVal.toObject();
        QString timestamp = decryptString(msgObj["timestamp"].toString());
        QString from = decryptString(msgObj["from"].toString());
        QString message = decryptString(msgObj["message"].toString());

        Qt::Alignment align = (from == LoggedUser) ? Qt::AlignRight : Qt::AlignLeft;
        QString displayFrom = (from == LoggedUser) ? "Me" : from;

        appendAlignedMessage(
            view,
            "[" + timestamp + "] " + displayFrom + ": " + message,
            align
            );
    }
}

void MainWindow::appendAlignedMessage(QTextEdit *view,
                                      const QString &text,
                                      Qt::Alignment alignment)
{
    if (!view) return;

    QTextCursor cursor = view->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(alignment);

    // Only insert new block if not empty
    if (!view->toPlainText().isEmpty()) {
        cursor.insertBlock(blockFormat);
    } else {
        cursor.setBlockFormat(blockFormat);
    }

    cursor.insertText(text);

    view->setTextCursor(cursor);
    view->ensureCursorVisible();
}

void MainWindow::clearCurrentChatHistory()
{
    QTextEdit *view = getCurrentChatView();
    if (!view) return;

    view->clear();      // Clears UI
    saveChatHistory();  // Updates JSON file
}

void MainWindow::setemojiBtn()
{
    emojiMenu = new QMenu(this);
    //smileys

    QMenu *smileyMenu = emojiMenu->addMenu("😊 Smileys");
    QStringList smileys = {
        "😀", "😃", "😄", "😁", "😆", "😅", "😂", "🤣",
        "😊", "😇", "🙂", "😉", "😌", "😍", "🥰", "😘",
        "😗", "😙", "😚", "😋", "😛", "😝", "😜", "🤪",
        "🤨", "🧐", "🤓", "😎", "🥳", "😏", "😒", "😞"
    };
    for (const QString &emoji : smileys)
    {
        QAction *action = smileyMenu->addAction(emoji);
        connect (action , &QAction::triggered , this , [this, emoji](){
            ui->messageEdit->insert(emoji);
            ui->messageEdit->setFocus();
        });
    }


    // Gestures & Hands
    QMenu *gesturesMenu = emojiMenu->addMenu("👍 Gestures");
    QStringList gestures = {
            "👍", "👎", "👌", "✌️", "🤞", "🤟", "🤘", "🤙",
            "👈", "👉", "👆", "👇", "☝️", "✋", "🤚", "🖐️",
            "🖖", "👋", "🤝", "👏", "🙌", "👐", "🤲", "🙏"
        };

    for (const QString &emoji : gestures) {
            QAction *action = gesturesMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Hearts & Love
    QMenu *heartsMenu = emojiMenu->addMenu("❤️ Hearts");
    QStringList hearts = {
            "❤️", "🧡", "💛", "💚", "💙", "💜", "🖤", "🤍",
            "🤎", "💔", "❣️", "💕", "💞", "💓", "💗", "💖",
            "💘", "💝", "💟"
        };

    for (const QString &emoji : hearts) {
            QAction *action = heartsMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Symbols & Objects
    QMenu *symbolsMenu = emojiMenu->addMenu("✨ Symbols");
    QStringList symbols = {
            "⭐", "✨", "💫", "💥", "💢", "💦", "💨", "🔥",
            "💯", "✔️", "✅", "❌", "❎", "⚠️", "🚫", "💤",
            "💬", "💭", "🗨️", "🗯️", "💡", "🔔", "🔕", "📢"
        };

    for (const QString &emoji : symbols) {
            QAction *action = symbolsMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Tech & Objects
    QMenu *objectsMenu = emojiMenu->addMenu("📱 Tech");
    QStringList objects = {
            "📱", "💻", "⌨️", "🖥️", "🖨️", "🖱️", "💾", "💿",
            "📀", "📷", "📹", "🎥", "📞", "☎️", "📟", "📠",
            "📺", "📻", "🎙️", "🎚️", "🎛️", "⏰", "⏱️", "⏲️"
        };

    for (const QString &emoji : objects) {
            QAction *action = objectsMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Food & Drink
    QMenu *foodMenu = emojiMenu->addMenu("🍕 Food");
    QStringList food = {
            "🍕", "🍔", "🍟", "🌭", "🍿", "🧂", "🥓", "🥚",
            "🍳", "🧇", "🥞", "🧈", "🍞", "🥐", "🥨", "🥯",
            "🥖", "🧀", "🥗", "🥙", "🥪", "🌮", "🌯", "🫔"
        };

    for (const QString &emoji : food) {
            QAction *action = foodMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Animals & Nature
    QMenu *animalsMenu = emojiMenu->addMenu("🐶 Animals");
    QStringList animals = {
            "🐶", "🐱", "🐭", "🐹", "🐰", "🦊", "🐻", "🐼",
            "🐨", "🐯", "🦁", "🐮", "🐷", "🐸", "🐵", "🐔",
            "🐧", "🐦", "🐤", "🦆", "🦅", "🦉", "🦇", "🐺"
        };

    for (const QString &emoji : animals) {
            QAction *action = animalsMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Activities & Sports
    QMenu *activitiesMenu = emojiMenu->addMenu("⚽ Activities");
    QStringList activities = {
            "⚽", "🏀", "🏈", "⚾", "🥎", "🎾", "🏐", "🏉",
            "🥏", "🎱", "🏓", "🏸", "🏒", "🏑", "🥍", "🏏",
            "🎯", "🎮", "🎲", "🎰", "🎳", "🎪", "🎨", "🎬"
        };

    for (const QString &emoji : activities) {
            QAction *action = activitiesMenu->addAction(emoji);
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Quick access separator
    emojiMenu->addSeparator();

        // Frequently used (flat list at bottom for quick access)
    QStringList favorites = {
            "😊", "😂", "❤️", "👍", "👋", "🎉", "🔥", "✨"
        };

    for (const QString &emoji : favorites) {
            QAction *action = emojiMenu->addAction(emoji + " ");
            connect(action, &QAction::triggered, this, [this, emoji]() {
                ui->messageEdit->insert(emoji);
                ui->messageEdit->setFocus();
            });
        }

        // Set menu to button
    ui->emojiBtn->setMenu(emojiMenu); //this automakes sure that it works on button press
}


/*---------FILE upload codes---------*/

void MainWindow::on_attachFile_clicked()
{
    if(hasattachedFile || !pendingFilePath.isEmpty()) //check if file already attached
    {
        // Clear internal state
        pendingFilePath.clear();
        hasattachedFile = false;

        // Reset message input placeholder
        ui->messageEdit->setPlaceholderText("Type a message...");

        // Reset attach button
        if (ui->attachFile) {
            ui->attachFile->setText("📎");
            ui->attachFile->setToolTip("Attach file");
        }
        // Hide status label
        if (ui->fileStatusLabel) {
            ui->fileStatusLabel->hide();
            ui->fileStatusLabel->setText("");
        }
        return;
    }
    //check reciever
    QString recipient = ui->activelist->currentText();
    if(recipient == "All"){
        QMessageBox::warning(this, "File Transfer",
                             "Cannot send files to 'All'. Please select a specific user.");
        return;
    }
    //open file browser
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Select FIle to Attach",
        QDir::homePath(),
        "All Files (*);;Documents (*.pdf *.docx *.txt);;Images (*.jpg *.png *.gif);;Videos (*.mp4 *.avi)"
        );

    if(filepath.isEmpty()) return; //user canceled
    //validate file
    QFileInfo fileinfo(filepath);
    qint64 filesize = fileinfo.size();

    if (filesize > 2147483648LL) {  // 2GB limit
        QMessageBox::warning(this, "File Too Large",
                             "File size exceeds 2GB limit.");
        return;
    }

    if(!fileinfo.exists() || !fileinfo.isReadable())
    {
        QMessageBox::critical(this, "Error",
                              "Cannot read file.");
        return;
    }

    //store file
    pendingFilePath = filepath;
    hasattachedFile = true;

    //update ui to show attachment
    QString filename = fileinfo.fileName();
    QString size = QString::number(filesize / 1024.0 /1024.0 ,'f' , 2) + " MB"; //convert to mb for displaying

    // Get icon based on file type
    QString icon = "📎";  // Default
    QString ext = fileinfo.suffix().toLower(); //gets the extension

    if (ext == "jpg" || ext == "png" || ext == "gif" || ext == "jpeg" || ext == "bmp") {
        icon = "🖼️";
    } else if (ext == "pdf") {
        icon = "📄";
    } else if (ext == "doc" || ext == "docx" || ext == "txt") {
        icon = "📝";
    } else if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov") {
        icon = "🎬";
    } else if (ext == "zip" || ext == "rar" || ext == "7z") {
        icon = "📦";
    } else if (ext == "mp3" || ext == "wav" || ext == "flac") {
        icon = "🎵";
    }

    //Update message input placeholder
    QString placeholderText = QString("%1 %2 (%3) - Type caption (optional)")
                                  .arg(icon)
                                  .arg(filename)
                                  .arg(size);
    ui->messageEdit->setPlaceholderText(placeholderText);

    // Change attach button to "remove" button
    if (ui->attachFile) {
        ui->attachFile->setText("✖");
        ui->attachFile->setToolTip("Remove attachment");
    }

    //Show a label with file info
    if (ui->fileStatusLabel) {
        ui->fileStatusLabel->setText(QString("%1 %2 (%3) attached")
                                         .arg(icon).arg(filename).arg(size));
        ui->fileStatusLabel->show();
    }
    //Show in chat ( so user knows file is ready)
    if (chatTabs.contains(recipient)) {
        chatTabs[recipient]->append(QString("--- File ready to send: %1 %2 (%3) ---")
                                        .arg(icon).arg(filename).arg(size));
    }
}

void MainWindow::sendFile(const QString &recipient, const QString &caption) //called in sendmsg func
{
    currentFilerecipient = recipient;
    // Check if already sending
    if (isSendingFile)
    {
        QMessageBox::warning(this, "File Transfer",
                             "You are already sending a file. Please wait.");
        return;
    }

    // Verify file still exists
    QFileInfo fileInfo(pendingFilePath);
    if (!fileInfo.exists()) {
        QMessageBox::critical(this, "Error", "File no longer exists.");
        return;
    }

    if (!fileInfo.isReadable()) {
        QMessageBox::critical(this, "Error", "File is not readable. Check permissions.");
        return;
    }

    QString fileName = fileInfo.fileName();
    qint64 fileSize = fileInfo.size();

    //send transfer request to server
    QString request = QString("FILE_TRANSFER_REQUEST:%1:%2:%3:%4:%5\n")
                          .arg(recipient)
                          .arg(LoggedUser)
                          .arg(fileName)
                          .arg(fileSize)
                          .arg(caption);
    tcpSocket->write(request.toUtf8());
    tcpSocket->flush();

    hasattachedFile = false;
    // Show waiting UI
    if (ui->fileStatusLabel) {
        ui->fileStatusLabel->setVisible(true);
        ui->fileStatusLabel->setText("Requesting file transfer...");
    }
}
void MainWindow::handleFileTransferBlocked(const QString &reason)
{
    isSendingFile = false;
    QMessageBox::warning(this, "File Transfer Blocked", reason);

    if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
    if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
    if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);
}

void MainWindow::handleFileTransferStart(const QString &transferId, const QString &sender,  //receiving file
                                         const QString &fileName, qint64 fileSize,
                                         const QString &caption)
{
    qDebug() << "Setting up file transfer:" << fileName << "from" << sender;

    // AUTO-ACCEPT (no blocking dialog)
    QString savePath = QDir::homePath() + "/Downloads/" + fileName;
    QDir().mkpath(QDir::homePath() + "/Downloads/");

    QFile *file = new QFile(savePath);
    if (!file->open(QIODevice::WriteOnly))
    {
        qDebug() << "ERROR: Cannot create file:" << file->errorString();
        QMessageBox::critical(this, "Error",
                              "Cannot create file: " + file->errorString());
        delete file;

        // Send rejection to server (optional)
        QString rejection = QString("FILE_TRANSFER_REJECTED:%1\n").arg(transferId);
        tcpSocket->write(rejection.toUtf8());
        tcpSocket->flush();
        return;
    }

    qDebug() << "File opened successfully:" << savePath;

    // Create incoming file structure (struct defined in mainwindow.h)
    IncomingFile incoming;
    incoming.file = file;
    incoming.fileName = fileName;
    incoming.sender = sender;
    incoming.totalSize = fileSize;
    incoming.bytesReceived = 0; //intital 0
    incoming.transferId = transferId;

    // Add to activeDownloads BEFORE receiving data
    activeDownloads[transferId] = incoming;

    qDebug() << "Added to activeDownloads. Waiting for data...";

    // Show progress UI
    if (ui->fileProgressBar) {
        ui->fileProgressBar->setVisible(true);
        ui->fileProgressBar->setValue(0);
        ui->fileProgressBar->setMaximum(100);
    }

    if (ui->fileStatusLabel) {
        ui->fileStatusLabel->setVisible(true);
        ui->fileStatusLabel->setText("Receiving " + fileName + "... 0%");
    }

    if (ui->cancelFileBtn) {
        ui->cancelFileBtn->setVisible(true);
    }

    // Show non-blocking notification
    QString notifMsg = sender + " is sending: " + fileName;
    if (!caption.isEmpty()) {
        notifMsg += "\n" + caption;
    }
    trayicon->showMessage("File incoming — NovaChat", notifMsg,
                          QSystemTrayIcon::Information, 3000);

    // Show in chat (non-blocking)
    if (chatTabs.contains(sender)) {
        chatTabs[sender]->append(QString("--- Receiving file: %1 (%2 MB) ---")
                                     .arg(fileName)
                                     .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2));
    }
}

void MainWindow::handleFileComplete(const QString &transferId)
{
    if (!activeDownloads.contains(transferId)) return;

    IncomingFile &incoming = activeDownloads[transferId]; //acces the file mapped by transferid

    if (incoming.file)
    {
        incoming.file->close();
        delete incoming.file;
    }
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm A");
    if (chatTabs.contains(incoming.sender))
    {
        QString displayMsg = QString("[%1] %2: 📎 Sent file: %3 (%4 MB)")
                                 .arg(timestamp)
                                 .arg(incoming.sender)
                                 .arg(incoming.fileName)
                                 .arg(incoming.totalSize / 1024.0 / 1024.0, 0, 'f', 2);

        appendAlignedMessage(chatTabs[incoming.sender], displayMsg, Qt::AlignLeft);
    }

    if (ui->fileStatusLabel) {
        ui->fileStatusLabel->setText("File received: " + incoming.fileName);
    }

    QTimer::singleShot(2000, this, [this]() {
        if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
        if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
        if (ui->cancelFileBtn) ui->cancelFileBtn->setVisible(false);
    });

    activeDownloads.remove(transferId);
    saveChatHistory();
}
void MainWindow::handleFileCancelled(const QString &transferId)
{
    if (activeDownloads.contains(transferId))
    {
        IncomingFile incoming = activeDownloads[transferId];

        if (incoming.file)
        {
            incoming.file->close();
            incoming.file->remove();  // Delete incomplete file
            delete incoming.file;
        }

        activeDownloads.remove(transferId);

        QMessageBox::information(this, "File Transfer",
                                 "File transfer was cancelled by the sender.");
    }

    if (ui->fileProgressBar) ui->fileProgressBar->setVisible(false);
    if (ui->fileStatusLabel) ui->fileStatusLabel->setVisible(false);
}

void MainWindow::handleFileData(const QString &transferId, const QByteArray &data)
{
    // This function is not used in current implementation
    // File data is handled directly in receiveMessage()
}
