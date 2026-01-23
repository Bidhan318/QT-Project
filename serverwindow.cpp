#include "serverwindow.h"
#include "ui_serverwindow.h"
#include "login_window.h"
#include "port.h"
#include <QHostAddress>   //for working with localhost,broadcast
#include <QTimer>
#include <QMessageBox>
#include <QThread>
#include <QTabBar>
#include <QNetworkInterface>
#include <QDateTime>

ServerWindow::ServerWindow(QString username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerWindow)
{
    ui->setupUi(this);
    Loggeduser = username;

    // Discovery timer - broadcast server presence
    discoveryTimer = new QTimer(this);
    connect(discoveryTimer, &QTimer::timeout, this, &ServerWindow::broadcastServerPresence);
    discoveryTimer->start(2000); // Every 2 seconds

    // UDP socket setup for presence announcements
    udpSocket = new QUdpSocket(this);
    bool bindResult = udpSocket->bind(QHostAddress::AnyIPv4, PORT,
                                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    if (!bindResult) {
        QMessageBox::warning(this, "UDP Error",
                             "Failed to bind UDP socket: " + udpSocket->errorString());
    }

    // Send initial discovery
    broadcastServerPresence();
    QTimer::singleShot(200, this, &ServerWindow::broadcastServerPresence);
    QTimer::singleShot(500, this, &ServerWindow::broadcastServerPresence);

    // TCP server setup for messaging
    tcpServer = new QTcpServer(this);

    // Start listening silently
    bool listenResult = tcpServer->listen(QHostAddress::Any, TCP_PORT);

    if (!listenResult) {
        QMessageBox::warning(this, "TCP Error",
                             "Failed to start TCP server: " + tcpServer->errorString());
    }

    connect(tcpServer, &QTcpServer::newConnection,
            this, &ServerWindow::onNewConnection);
    connect(ui->msg_in, &QLineEdit::returnPressed,
            this, &ServerWindow::on_send_btn_clicked);

    connect(ui->client_tab->tabBar(), &QTabBar::tabCloseRequested,
            this, &ServerWindow::onTabscloseRequested);
}

ServerWindow::~ServerWindow()
{
    // Stop the discovery timer
    if (discoveryTimer) {
        discoveryTimer->stop();
    }

    // Close all client connections
    for (QTcpSocket *socket : clientSockets.keys()) //clientSockets is qmap storing connected clients
    {
        if (socket) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    }

    // Close TCP server
    if (tcpServer) {
        tcpServer->close();
    }

    delete ui;
}

void ServerWindow::onNewConnection()  //called auto when a new client connects
{
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();  //returns socket obj for new client

    if (!clientSocket) return;

    connect(clientSocket, &QTcpSocket::readyRead,
            this, &ServerWindow::onClientDataReceived);
    connect(clientSocket, &QTcpSocket::disconnected,
            this, &ServerWindow::onClientDisconnected);

    // Store socket temporarily until we get username
    clientSockets.insert(clientSocket, "");
}

void ServerWindow::onClientDataReceived() //this routes the msg from one client to others
{
    //identifies which client sent data
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());  //sender returns pointer to the one that emitted signal

    if (!clientSocket) return;
    // If socket is in file-transfer mode, relay raw bytes ONLY
    if (socketToTransferId.contains(clientSocket)) {
        QString transferId = socketToTransferId[clientSocket];
        if (activeTransfers.contains(transferId)) {
            handleFileDataRelay(clientSocket, transferId);
            return; //do not parse as text
        }
    }

    //msg handling
    QByteArray data = clientSocket->readAll(); //reads all data from this client
    // Only convert to QString if it's NOT file data
    QString alldata = QString::fromUtf8(data);

    //split by new lines in case multiple msg arrive
    QStringList messages = alldata.split('\n', Qt::SkipEmptyParts);

    for (const QString &message : messages)
    {
        if(message.isEmpty()) continue;

        //handle login
        if(message.startsWith("LOGIN:"))
        {
            QString username = message.mid(6).trimmed();
            if(username.isEmpty()) continue;

            clientSockets[clientSocket] = username; //update socket qmap
            addClientTab(username);

            if(ui->client_name)
            {
                ui->client_name->append(username + " has joined the chat");
            }
            continue;
        }
        //handle logout
        if(message.startsWith("LOGOUT:"))
        {

            QString username = message.mid(7).trimmed();
            if(username.isEmpty()) continue;

            if(ui->client_name)
            {
                ui->client_name->append(username + " has left the chat");
            }

            removeClientTab(username);
            clientSockets.remove(clientSocket);

            clientSocket->disconnectFromHost();
            continue;
        }

        //handle file transfer request
        if(message.startsWith("FILE_TRANSFER_REQUEST:"))
        {
            QStringList parts = message.split(':');
            if (parts.size() >= 5)
            {
                QString recipient = parts[1];
                QString sender = parts[2];
                QString fileName = parts[3];
                qint64 fileSize = parts[4].toLongLong();
                QString caption = parts.size() > 5 ? parts.mid(5).join(':') : "";

                // Check if transfer can start
                if(!canStartTransfer(sender, recipient))
                {
                    QString rejection;
                    if (busySenders.contains(sender)) {
                        rejection = "FILE_TRANSFER_BLOCKED:You are already sending a file. Please wait.\n";
                    } else if (busyRecipients.contains(recipient)) {
                        rejection = QString("FILE_TRANSFER_BLOCKED:%1 is currently receiving a file. Try again later.\n")
                        .arg(recipient);
                    }
                    clientSocket->write(rejection.toUtf8());
                    clientSocket->flush();
                    continue;
                }

                // Generate unique transfer ID
                QString transferId = generateTransferId(sender, recipient);

                // Create transfer info
                FileTransferInfo info;
                info.sender = sender;
                info.recipient = recipient;
                info.fileName = fileName;
                info.fileSize = fileSize;
                info.bytesTransferred = 0;
                info.expectedSize = fileSize;
                info.transferId = transferId;
                //store in pending not started yet
                pendingTransfers[transferId] = info;

                // Mark busy BEFORE sending approval
                busySenders.insert(sender);
                busyRecipients.insert(recipient);

                // Display in sender's tab
                if(clientTabs.contains(sender))
                {
                    QTextEdit *view = clientTabs[sender];
                    if(view)
                    {
                        QString displayMsg = QString("%1 (to %2): 📎 Requesting to send: %3 (%4 MB)")
                                                 .arg(sender)
                                                 .arg(recipient)
                                                 .arg(fileName)
                                                 .arg(fileSize / 1024.0 / 1024.0, 0, 'f', 2);

                        if (!caption.isEmpty()) {
                            displayMsg += "\n    Caption: " + caption;
                        }
                        view->append(displayMsg);
                    }
                }

                //Find recipient socket first
                QTcpSocket *recipientSocket = nullptr;
                for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
                {
                    if(it.value() == recipient)
                    {
                        recipientSocket = it.key();
                        break;
                    }
                }

                if (!recipientSocket || recipientSocket->state() != QAbstractSocket::ConnectedState)
                {
                    // Recipient offline - abort
                    clientSocket->write("FILE_TRANSFER_BLOCKED:Recipient is offline.\n");
                    clientSocket->flush();

                    activeTransfers.remove(transferId);
                    busySenders.remove(sender);
                    busyRecipients.remove(recipient);
                    continue;
                }

                // Send PENDING APPROVAL to recipient FIRST
                QString pendingmsg = QString("FILE_TRANSFER_PENDING:%1:%2:%3:%4:%5\n")
                                         .arg(transferId)
                                         .arg(sender)
                                         .arg(fileName)
                                         .arg(fileSize)
                                         .arg(caption);

                recipientSocket->write(pendingmsg.toUtf8());
                recipientSocket->flush();

                // Give recipient time to create file
                QThread::msleep(100);

                // NOW enable relay mode for sender (AFTER recipient is ready)
                socketToTransferId[clientSocket] = transferId;

                // Tell sender to wait
                QString waitMsg = QString("FILE_TRANSFER_WAITING:%1\n").arg(transferId);
                clientSocket->write(waitMsg.toUtf8());
                clientSocket->flush();
            }
            continue;
        }

        //if file transfer accepted by the reciever
        if(message.startsWith("FILE_TRANSFER_ACCEPTED:"))
        {
            QString transferId = message.mid(23).trimmed();

            if (!pendingTransfers.contains(transferId)) {
                continue; // Already processed or expired
            }

            FileTransferInfo info = pendingTransfers[transferId];

            // Move from pending to active
            activeTransfers[transferId] = info;
            pendingTransfers.remove(transferId);

            // Mark busy
            busySenders.insert(info.sender);
            busyRecipients.insert(info.recipient);

            // Find sender socket
            QTcpSocket *senderSocket = nullptr;
            for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
            {
                if(it.value() == info.sender)
                {
                    senderSocket = it.key();
                    break;
                }
            }

            if (!senderSocket || senderSocket->state() != QAbstractSocket::ConnectedState)
            {
                // Sender disconnected - notify recipient
                for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
                {
                    if(it.value() == info.recipient)
                    {
                        QTcpSocket *recipientSocket = it.key();
                        recipientSocket->write("FILE_TRANSFER_BLOCKED:Sender is no longer online.\n");
                        recipientSocket->flush();
                        break;
                    }
                }
                activeTransfers.remove(transferId);
                busySenders.remove(info.sender);
                busyRecipients.remove(info.recipient);
                continue;
            }

            // Find recipient socket
            QTcpSocket *recipientSocket = nullptr;
            for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
            {
                if(it.value() == info.recipient)
                {
                    recipientSocket = it.key();
                    break;
                }
            }

            // Send FILE_TRANSFER_START to recipient
            QString fileHeader = QString("FILE_TRANSFER_START:%1:%2:%3:%4:%5\n")
                                     .arg(transferId)
                                     .arg(info.sender)
                                     .arg(info.fileName)
                                     .arg(info.fileSize)
                                     .arg(""); // Caption already shown in pending dialog

            recipientSocket->write(fileHeader.toUtf8());
            recipientSocket->flush();

            // Give recipient time to create file
            QThread::msleep(100);

            // Enable relay mode for sender
            socketToTransferId[senderSocket] = transferId;

            // Send approval to sender to start sending
            QString approval = QString("FILE_TRANSFER_APPROVED:%1\n").arg(transferId);
            senderSocket->write(approval.toUtf8());
            senderSocket->flush();

            continue;
        }

        // Add NEW handler for rejection - REJECTED
        if(message.startsWith("FILE_TRANSFER_REJECTED:"))
        {
            QString transferId = message.mid(23).trimmed();

            if (!pendingTransfers.contains(transferId)) {
                continue;
            }

            FileTransferInfo info = pendingTransfers[transferId];
            pendingTransfers.remove(transferId);

            busySenders.remove(info.sender);
            busyRecipients.remove(info.recipient);
            //clean up socket mapping
            for (auto it = socketToTransferId.begin(); it != socketToTransferId.end(); )
            {
                if (it.value() == transferId)
                    it = socketToTransferId.erase(it);
                else
                    ++it;
            }

            // Notify sender
            for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
            {
                if(it.value() == info.sender)
                {
                    QTcpSocket *senderSocket = it.key();
                    QString rejection = QString("FILE_TRANSFER_BLOCKED:%1 declined to receive the file.\n")
                                            .arg(info.recipient);
                    senderSocket->write(rejection.toUtf8());
                    senderSocket->flush();
                    break;
                }
            }

            // Update sender's tab
            if(clientTabs.contains(info.sender))
            {
                clientTabs[info.sender]->append(
                    QString("%1 declined file: %2")
                        .arg(info.recipient)
                        .arg(info.fileName)
                    );
            }

            continue;
        }
        //handle file transfer complete
        if(message.startsWith("FILE_TRANSFER_COMPLETE:"))
        {
            QString transferId = message.mid(23).trimmed();
            if (activeTransfers.contains(transferId))
            {
                FileTransferInfo info = activeTransfers[transferId];

                // Clean up
                busySenders.remove(info.sender);
                busyRecipients.remove(info.recipient);
                activeTransfers.remove(transferId);

                // Remove socket mappings
                for (auto it = socketToTransferId.begin(); it != socketToTransferId.end();)
                {
                    if (it.value() == transferId) {
                        it = socketToTransferId.erase(it);
                    } else {
                        ++it;
                    }
                }

                for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
                {
                    if(it.value() == info.recipient)
                    {
                        QTcpSocket *recipientSocket = it.key();
                        if(recipientSocket->state() == QAbstractSocket::ConnectedState)
                        {
                            recipientSocket->write((message + "\n").toUtf8());
                            recipientSocket->flush();
                        }
                        break;
                    }
                }
            }
            continue;
        }

        //get username for this socket from qmap
        QString senderUsername = clientSockets.value(clientSocket, "");
        if(senderUsername.isEmpty()) continue; //not logged in yet

        //parse msg format
        QStringList parts = message.split(':');
        if(parts.size() < 3) continue;

        QString msgtype = parts[0];

        if(msgtype == "BROADCAST")
        {
            QString sender = parts[1];
            QString text = parts.mid(2).join(':');

            //display in senders tab
            if(clientTabs.contains(sender))
            {
                QTextEdit *view = clientTabs[sender];
                if(view)
                {
                    view->append(sender + ": " + text);
                }
            }

            //broadcast to all other clients
            QString broadcastmsg = "BROADCAST:" + sender + ":" + text + "\n";
            for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it) //for all clients
            {
                QTcpSocket *socket = it.key();
                QString username = it.value();

                if(!username.isEmpty() && socket->state() == QAbstractSocket::ConnectedState)
                {
                    socket->write(broadcastmsg.toUtf8());
                    socket->flush();
                }
            }
        }

        else if (msgtype == "PRIVATE")
        {
            QString recipient = parts[1];
            QString sender = parts[2];
            QString text = parts.mid(3).join(':');

            //display in senders tab
            if(clientTabs.contains(sender))
            {
                QTextEdit *view = clientTabs[sender];
                if(view)
                {
                    view->append(sender + " (to " + recipient + "): " + text);
                }
            }

            //forward msg to reciever
            QString pvtmsg = "PRIVATE:" + sender + ":" + text + "\n";
            for(auto it = clientSockets.begin(); it != clientSockets.end() ; ++it)
            {
                if(it.value() == recipient)
                {
                    QTcpSocket *recipientsocket = it.key();
                    if(recipientsocket->state() == QAbstractSocket::ConnectedState)
                    {
                        recipientsocket->write(pvtmsg.toUtf8());
                        recipientsocket->flush();
                    }
                    break;
                }
            }

        }
    }
}

void ServerWindow::broadcastServerPresence() //checked in mainwindow
{
    if (!udpSocket) return;

    // Get server's local IP address
    QString serverIP = "127.0.0.1";  // Default to localhost
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost &&
            !address.toString().startsWith("169.254")) {  // Ignore auto-IP addresses (link-local)
            serverIP = address.toString();
            break;  // Use first valid private IP found
        }
    }

    QString discoveryMsg = "SERVER_DISCOVERY:" + serverIP;  // Include IP for clients to find us
    udpSocket->writeDatagram(
        discoveryMsg.toUtf8(),
        QHostAddress::Broadcast,
        PORT
        );
}

void ServerWindow::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QString username = clientSockets.value(clientSocket, "");

    if (!username.isEmpty())
    {
        cleanupUserTransfers(username); //clean any active transfers
        removeClientTab(username); //func def below

        if (ui->client_name) {
            ui->client_name->append(username + " has left the chat");
        }
    }

    clientSockets.remove(clientSocket); //remove from qmap
    clientSocket->deleteLater();
}

void ServerWindow::addClientTab(const QString &username)
{
    if (clientTabs.contains(username)) return;  //check if tab already exists for the username

    if (!ui->client_tab) return;

    QTextEdit *view = new QTextEdit;
    view->setReadOnly(true); //set so that it only sees and doesn't take input
    view->setPlaceholderText("Messages appear here...");
    ui->client_tab->addTab(view, username);
    ui->client_tab->setTabsClosable(true);
    clientTabs.insert(username, view);
}

void ServerWindow::removeClientTab(const QString &username)
{
    if (!clientTabs.contains(username)) return; //check if no tab exists for the username in the qmap

    QTextEdit *view = clientTabs.value(username); //returns the specific tab of the username
    //----safety checks--
    if (!view) {
        clientTabs.remove(username);
        return;
    }

    if (!ui->client_tab) {
        clientTabs.remove(username);
        delete view;
        return;
    }
    //--------------------
    int index = ui->client_tab->indexOf(view); //Finds which tab number this specific username is in. Tabs are numbered: 0, 1, 2, 3, ...

    if (index != -1) //checks if the tab is found
    {
        ui->client_tab->removeTab(index);
    }

    clientTabs.remove(username); //remove username from tracking qmap
    delete view;
}


void ServerWindow::on_send_btn_clicked()
{
    QString msg = ui->msg_in->text();
    if (msg.isEmpty()) return;

    // Broadcast server message to all clients
    QString servermsg = "SERVER:" + msg + "\n";

    //send msgs in tcp for all clients
    for(auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
    {
        QTcpSocket *socket = it.key();
        QString username = it.value();
        if (!username.isEmpty() && socket->state() == QAbstractSocket::ConnectedState)
        {
            socket->write(servermsg.toUtf8());
            socket->flush();
        }

    }

    ui->msg_in->clear();
}

void ServerWindow::on_disconnect_btn_clicked()
{
    //stop broadcasting server presence
    if(discoveryTimer)
    {
        discoveryTimer->stop();
    }

    // Send shutdown signal to all clients via TCP
    QString shutdownMsg = "SERVER_SHUTDOWN\n";

    for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
    {
        QTcpSocket *socket = it.key();
        if (socket && socket->state() == QAbstractSocket::ConnectedState)
        {
            socket->write(shutdownMsg.toUtf8());
            socket->flush();
            socket->waitForBytesWritten(1000);
        }
    }

    // Give clients time to receive the shutdown message
    QThread::msleep(100);

    //notify and disconnect all clients
    for(QTcpSocket* socket : clientSockets.keys())
    {
        if(socket && socket->state()== QAbstractSocket::ConnectedState)
        {
            socket->disconnectFromHost();
            socket->waitForDisconnected(1000);
        }
    }

    //clear all client tabs
    for (const QString &username : clientTabs.keys())
    {
        QTextEdit *view = clientTabs[username];
        if (view) {
            int index = ui->client_tab->indexOf(view);
            if (index != -1) {
                ui->client_tab->removeTab(index);
            }
            delete view;
        }
    }

    // Clear the QMaps
    clientSockets.clear();  // Clears all socket-username mappings
    clientTabs.clear();     // Clears all username-tab mappings

    // Update UI
    if (ui->client_name) {
        ui->client_name->append("Server disconnected - All clients removed");
    }

    // Restart discovery for future connections
    if(discoveryTimer)
    {
        discoveryTimer->start(2000);
    }
}

void ServerWindow::onTabscloseRequested(int index)
{
    if (!ui->client_tab) return;

    QString username = ui->client_tab->tabText(index);

    if (username.isEmpty()) return;

    // Find socket for this user
    for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
    {
        if (it.value() == username)  //finds index of specific tab in qmap
        {
            QTcpSocket *socket = it.key();  //.key returns qtcp pointer to the object

            if (socket && socket->state() == QAbstractSocket::ConnectedState)
            {
                socket->write("KICKED\n"); //sends msg in tcpsocket and checked in mainwindow::recievemsg
                socket->flush();
                socket->disconnectFromHost();
                socket->waitForDisconnected(1000);
            }

            clientSockets.remove(socket);
            break;
        }
    }
}

void ServerWindow::on_logout_clicked()
{
    on_disconnect_btn_clicked(); //because in tcp if server closes msg passing is not gonna work anyway

    if(discoveryTimer)
    {
        disconnect(discoveryTimer, nullptr, nullptr, nullptr); // Disconnect signals
    }

    if(udpSocket)
    {
        udpSocket->close(); // Close UDP socket permanently
    }

    if(tcpServer)
    {
        tcpServer->close(); // Close TCP server permanently
    }

    loginwindow = new Login_Window();
    loginwindow->show();
    close();
    deleteLater();
}

/*--------file transfer helper funcs--------*/
QString ServerWindow::generateTransferId(const QString &sender, const QString &recipient)
{
    return sender + "_to_" + recipient + "_" +
           QString::number(QDateTime::currentMSecsSinceEpoch());
}

bool ServerWindow::canStartTransfer(const QString &sender, const QString &recipient)
{
    // Check if sender is already sending
    if (busySenders.contains(sender)) {
        return false;
    }

    // Check if recipient is already receiving
    if (busyRecipients.contains(recipient)) {
        return false;
    }

    return true;
}

//this funcroutes binary file chunks from the sender to the
// correct recipient without mixing them up with other transfers.
void ServerWindow::handleFileDataRelay(QTcpSocket *senderSocket, const QString &transferId)
{
    if (!activeTransfers.contains(transferId)) {
        qDebug() << "Transfer ID not found:" << transferId;
        return;
    }

    FileTransferInfo &info = activeTransfers[transferId]; //define structure and assign to incoming file

    // Read all available data from sender
    QByteArray chunk = senderSocket->readAll();

    if (chunk.isEmpty()) {
        qDebug() << "Empty chunk received";
        return;
    }

    qDebug() << "Relaying" << chunk.size() << "bytes for" << transferId;

    // Find recipient socket
    QTcpSocket *recipientSocket = nullptr;
    for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it)
    {
        if (it.value() == info.recipient)
        {
            recipientSocket = it.key();  //find the recipients socket
            break;
        }
    }

    if (!recipientSocket || recipientSocket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Recipient socket stnot available!";

        // Cancel transfer
        QString cancellation = QString("FILE_TRANSFER_CANCELLED:%1\n").arg(transferId);
        senderSocket->write(cancellation.toUtf8());
        senderSocket->flush();

        // Cleanup
        busySenders.remove(info.sender);
        busyRecipients.remove(info.recipient);
        activeTransfers.remove(transferId);
        socketToTransferId.remove(senderSocket);
        return;
    }

    // Forward to recipient with error checking
    qint64 written = recipientSocket->write(chunk);
    if (written != chunk.size()) {
        qDebug() << "Warning: Only wrote" << written << "of" << chunk.size() << "bytes";
    }
    recipientSocket->flush();

    //Track how much we've RECEIVED from sender, not sent to recipient
    info.bytesTransferred += chunk.size();  // This tracks RECEIVED bytes

    qDebug() << "Progress:" << info.bytesTransferred << "/" << info.expectedSize;

    // Check if transfer complete
    if (info.bytesTransferred >= info.expectedSize)
    {
        qDebug() << "Transfer complete:" << transferId;

        // Give time for last chunk to reach recipient
        QThread::msleep(100);

        QString completeMsg = QString("FILE_TRANSFER_COMPLETE:%1\n").arg(transferId);

        // Notify both parties
        senderSocket->write(completeMsg.toUtf8());
        senderSocket->flush();

        recipientSocket->write(completeMsg.toUtf8());
        recipientSocket->flush();

        // Cleanup
        busySenders.remove(info.sender);
        busyRecipients.remove(info.recipient);
        activeTransfers.remove(transferId);
        socketToTransferId.remove(senderSocket);
    }
}

void ServerWindow::cleanupUserTransfers(const QString &username)
{
    // Remove from busy sets
    busySenders.remove(username);
    busyRecipients.remove(username);

    // Cancel active transfers involving this user
    for (auto it = activeTransfers.begin(); it != activeTransfers.end();)
    {
        if (it->sender == username || it->recipient == username)
        {
            QString transferId = it.key();

            // Notify the other party
            QString otherUser = (it->sender == username) ? it->recipient : it->sender;
            QString cancellation = QString("FILE_TRANSFER_CANCELLED:%1\n").arg(transferId);

            for(auto sockIt = clientSockets.begin(); sockIt != clientSockets.end(); ++sockIt)
            {
                if(sockIt.value() == otherUser)
                {
                    QTcpSocket *socket = sockIt.key();
                    if(socket->state() == QAbstractSocket::ConnectedState)
                    {
                        socket->write(cancellation.toUtf8());
                        socket->flush();
                    }
                    break;
                }
            }

            it = activeTransfers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Clean up socket mappings
    for (auto it = socketToTransferId.begin(); it != socketToTransferId.end();)
    {
        QTcpSocket *socket = it.key();
        if (clientSockets.value(socket, "") == username)
        {
            it = socketToTransferId.erase(it);
        }
        else
        {
            ++it;
        }
    }
}



