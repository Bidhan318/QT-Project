#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include "port.h"
#include <QSystemTrayIcon>
#include <QIcon>
#include <QTimer>
#include <QString>
#include <QNetworkInterface>
#include <QDebug>

MainWindow::MainWindow(QString username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , LoggedUser(username)
{
    ui->setupUi(this);

    //----TCP for online detection and UDP for msg passing----------------

    // UDP socket setup for messaging
    udpSocket = new QUdpSocket(this);  //using this makes mainwindow parent so socket closes when the window is closed
    udpSocket->bind(QHostAddress::AnyIPv4, PORT,
                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    // TCP socket setup for presence detection
    tcpSocket = new QTcpSocket(this);

    // Set up periodic presence announcement (every 3 seconds)
    QTimer *announcementTimer = new QTimer(this);
    connect(announcementTimer, &QTimer::timeout, this, &MainWindow::announcePresence);
    announcementTimer->start(3000); // Announce every 3 seconds

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
            this, &MainWindow::receiveMessage);  //signal when udp packet arrives

    // TCP connections
    connect(tcpSocket, &QTcpSocket::connected,  //signal when tcp connection establishes
            this, &MainWindow::onConnectedToServer);
    connect(tcpSocket, &QTcpSocket::disconnected,
            this, &MainWindow::onDisconnectedFromServer);
    connect(tcpSocket, &QTcpSocket::errorOccurred,
            this, &MainWindow::onTcpError);

    // Connect to server
    connectToServer();
}

MainWindow::~MainWindow()
{
    // Notify server of disconnect
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
        tcpSocket->waitForDisconnected(1000); //waits 1000ms = 1sec for disconnect to complete
    }
    delete ui;
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
    QString loginMsg = "LOGIN:" + LoggedUser;
    tcpSocket->write(loginMsg.toUtf8());  //converts qstring to byte array for sending
    tcpSocket->flush();  //force immediate sending of data
}

void MainWindow::onDisconnectedFromServer() //for when cross pressed by server in tab list
{
    QByteArray data = tcpSocket->readAll(); //receives from serverwindow::addClientTab where if the cross is pressed sends "kicked" msg
    QString msg = QString::fromUtf8(data);

    if (msg == "kicked") {
        if (chatTabs.contains("All")) {
            chatTabs["All"]->append("--- You have been disconnected by the server ---");
        }

        ui->messageEdit->setEnabled(false);
        ui->sendBtn->setEnabled(false);

        if (ui->activelist) {
            ui->activelist->setEnabled(false);
        }

        if (udpSocket->state() == QAbstractSocket::BoundState) {
            udpSocket->close();
        }

        return;
    }
    if (chatTabs.contains("All")) {
        chatTabs["All"]->append("--- Disconnected from server ---");
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
    if (msg.isEmpty()) return;

    QString destination = ui->activelist->currentText(); //accesses the text of drop down active list

    if(destination=="All")
    {
        // Send broadcast message to everyone
        QString fullMsg = LoggedUser + ": " + msg;
        udpSocket->writeDatagram(
            fullMsg.toUtf8(),
            QHostAddress::Broadcast,
            PORT
            );
        //show in all tab
        if(chatTabs.contains("All"))
        {
            chatTabs["All"]->append("Me: "+ msg);
        }
    }
    else{
        //send private messages
        if(activeClients.contains(destination))
        {
            QString privatemsg = LoggedUser + " (to " + destination + " only): " + msg;
            QHostAddress targetIP = activeClients.value(destination);

            // Get my own IP for comparison
            QString myIP = "127.0.0.1";
            foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol &&
                    address != QHostAddress::LocalHost &&
                    !address.toString().startsWith("169.254")) {
                    myIP = address.toString();
                    break;
                }
            }

            // If target IP matches my IP, both are on same machine - use broadcast
            if(targetIP.toString() == myIP ||
                targetIP == QHostAddress::LocalHost ||
                targetIP.toString() == "127.0.0.1")
            {
                // Same device - use broadcast
                udpSocket->writeDatagram(privatemsg.toUtf8(),
                                         QHostAddress::Broadcast,
                                         PORT);
            }
            else
            {
                // Different device - send directly
                udpSocket->writeDatagram(privatemsg.toUtf8(),
                                         targetIP,
                                         PORT);
            }

            if(chatTabs.contains(destination))
            {
                chatTabs[destination]->append("Me: "+ msg);
            }
        }
    }
    ui->messageEdit->clear();
}

void MainWindow::receiveMessage()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram; //creates bytearray to store received data
        datagram.resize(udpSocket->pendingDatagramSize()); //resize to next packet's size in bytes

        QHostAddress senderIP; //defines user's info
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(),  //network auto fills senderIP with sender ip address
                                &senderIP, &senderPort);
        QString msg = QString::fromUtf8(datagram);

        // Handle server shutdown message
        if (msg == "SERVER_SHUTDOWN")
        {
            //append to all tabs possible
            for (QTextEdit *view : chatTabs.values()) {
                if (view) {
                    view->append("--- Server has disconnected ---");
                    view->setEnabled(false);  // Disable all chat views
                }
            }

            ui->messageEdit->setEnabled(false);
            ui->sendBtn->setEnabled(false);

            //disable the list to choose people
            if(ui->activelist)
            {
                ui->activelist->setEnabled(false);
            }

            // Disconnect TCP
            if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                tcpSocket->disconnectFromHost();
            }

            // Close UDP socket to stop receiving messages
            udpSocket->close();

            continue;
        }

        // Check if this is a server discovery message - code in serverwindow.cpp
        if (msg.startsWith("SERVER_DISCOVERY"))
        {
            // Extract server IP from discovery message
            QString serverIP = "127.0.0.1";  // Default
            if (msg.contains(":")) {
                serverIP = msg.mid(msg.indexOf(':') + 1).trimmed();
            }

            // Re-register with server if not connected
            if (tcpSocket->state() != QAbstractSocket::ConnectedState)
            {
                // Disconnect first if needed
                if (tcpSocket->state() != QAbstractSocket::UnconnectedState) {
                    tcpSocket->abort();
                }
                // Connect to the discovered server IP
                tcpSocket->connectToHost(QHostAddress(serverIP), TCP_PORT);
            }
            continue; // Don't display this message
        }

        //handle client announce messages
        if(msg.startsWith("CLIENT_ANNOUNCE:"))
        {
            QString announcedUser = msg.mid(16).trimmed(); //trims the username right to the :
            //don't add yourself in the active list
            if(announcedUser!=LoggedUser && announcedUser!= "Server"){
                // Always update IP address in case user reconnected from different IP
                activeClients[announcedUser] = senderIP;

                // Only add UI elements if this is a new user
                if(!chatTabs.contains(announcedUser))
                {
                    if(ui->activelist){
                        ui->activelist->addItem(announcedUser); //add the user to activelist
                    }
                    addChatTab(announcedUser);
                }
            }
            continue;
        }

        //extract username and check if it's private
        int sep = msg.indexOf(':');
        if(sep!=-1)
        {
            QString sender = msg.left(sep).trimmed();

            // Check if this is your own message (before cleaning)
            // This prevents seeing your own broadcast messages twice
            if(sender == LoggedUser) {
                continue;  // Ignore your own messages
            }

            QString message = msg.mid(sep+1).trimmed();

            //
            bool isPrivate = sender.contains(" (to " + LoggedUser + " only)");

            if(sender.contains(" (to ") && sender.contains(" only)") && !isPrivate)
            {
                continue;  // Ignore private messages meant for others
            }

            bool fromServer = sender.startsWith("Server");

            // Clean username for display (remove private message markers)
            QString cleanUsername = sender;
            cleanUsername.remove(" (to " + LoggedUser + " only)");  //msg is in form of: Ram (to hari only): msg . so we remove the excess part
            cleanUsername.remove(" (private)");
            cleanUsername = cleanUsername.trimmed();

            //Route msgs to appropriate tabs
            if(!msg.startsWith(LoggedUser + ":"))
            {
                if(isPrivate)
                {
                    //Ensure tab exists for private messages (in case it arrives before CLIENT_ANNOUNCE)
                    if(!chatTabs.contains(cleanUsername))
                    {
                        addChatTab(cleanUsername);
                        if(ui->activelist && !activeClients.contains(cleanUsername)){
                            ui->activelist->addItem(cleanUsername);
                            activeClients.insert(cleanUsername, senderIP);
                        }
                    }

                    // Display private message in sender's private tab
                    if(chatTabs.contains(cleanUsername))
                    {
                        chatTabs[cleanUsername]->append(cleanUsername+": " +message);
                    }
                }
                else if(fromServer)
                {
                    // Server messages go to "All" tab
                    if(chatTabs.contains("All"))
                    {
                        chatTabs["All"]->append(cleanUsername+": " +message);
                    }
                }
                else
                {
                    // Regular broadcast messages go to "All" tab
                    //Don't show SERVER_DISCOVERY messages in All tab
                    if(cleanUsername != "SERVER_DISCOVERY") {
                        if(chatTabs.contains("All"))
                        {
                            chatTabs["All"]->append(cleanUsername+": " +message);
                        }
                    }
                }

                //----for notifications---
                if (windowState() & Qt::WindowMinimized || !isActiveWindow()) //windowState() returns if window is minimized or maximized
                {
                    trayicon->showMessage(
                        "New message — NovaChat",
                        cleanUsername + ": "+message, QSystemTrayIcon::Information,
                        3000 //notification for 3 sec
                        );
                }
            }
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
    chatTabs.remove(username);
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
    udpSocket->writeDatagram(announce.toUtf8(),QHostAddress::Broadcast,PORT);
}
