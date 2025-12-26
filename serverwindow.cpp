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

    // UDP socket setup for receiving messages
    udpSocket = new QUdpSocket(this);
    bool bindResult = udpSocket->bind(QHostAddress::AnyIPv4, PORT,
                                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    if (!bindResult) {
        QMessageBox::warning(this, "UDP Error",
                             "Failed to bind UDP socket: " + udpSocket->errorString());
    }

    // Send initial discovery
    broadcastServerPresence();

    // TCP server setup for presence detection
    tcpServer = new QTcpServer(this);

    // Start listening silently
    bool listenResult = tcpServer->listen(QHostAddress::Any, TCP_PORT);

    if (!listenResult) {
        QMessageBox::warning(this, "TCP Error",
                             "Failed to start TCP server: " + tcpServer->errorString());
    }

    connect(udpSocket, &QUdpSocket::readyRead,
            this, &ServerWindow::receiveMessage);
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

void ServerWindow::onClientDataReceived()
{
    //identifies which client sent data
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());  //sender returns pointer to the one that emitted signal

    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll(); //reads all data from this client
    QString message = QString::fromUtf8(data);

    // Expected format: "LOGIN:username"
    if (message.startsWith("LOGIN:"))  //received from MainWindow::onConnectedToServer
    {
        QString username = message.mid(6).trimmed();  //.mid(6): Extracts substring starting at index 6. "LOGIN:ram" → starts at index 6 → "ram"

        if (username.isEmpty()) return;

        // Update socket mapping
        clientSockets[clientSocket] = username; //changes the empty space in newConnection func to username

        // Add tab for this client
        addClientTab(username); //func def below

        if (ui->client_name) {
            ui->client_name->append(username + " has joined the chat");
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

void ServerWindow::receiveMessage()
{
    if (!udpSocket) return;  //safety check

    while (udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());  //change the datagram size to next udp packet's size
        udpSocket->readDatagram(datagram.data(), datagram.size());
        QString msg = QString::fromUtf8(datagram);

        if(msg.startsWith("CLIENT_ANNOUNCE")) //ignore client announce messages
        {
            continue;
        }

        // Format: username: message
        int sep = msg.indexOf(':'); //finds index of colon(:) in the sent format msg
        if (sep == -1) continue; //-1 if not found

        QString username = msg.left(sep).trimmed(); //trims the left part that is username
        QString text = msg.mid(sep + 1).trimmed();  //trims the msg part

        if (username.contains("(to ") && username.contains(" only)")) {
            continue;  // Skip private messages, don't display them on server
        }

        // Clean username for display
        QString cleanUsername = username;
        cleanUsername.remove(" (private)");
        cleanUsername = cleanUsername.trimmed();

        // Append message to correct tab if it exists
        if (clientTabs.contains(cleanUsername))
        {
            QTextEdit *view = clientTabs[cleanUsername]; //edit in correct tab
            if (view) {
                view->append(cleanUsername + ": " + text); //append the text in the tab of the user
            }
        }
    }
}

void ServerWindow::on_send_btn_clicked()
{
    QString msg = ui->msg_in->text();
    if (msg.isEmpty()) return;

    // Broadcast server message to all clients
    QString fullmsg = "Server: " + msg;
    udpSocket->writeDatagram(fullmsg.toUtf8(),
                             QHostAddress::Broadcast, PORT);
    ui->msg_in->clear();
}

void ServerWindow::on_disconnect_btn_clicked()
{
    //stop broadcasting server presence
    if(discoveryTimer)
    {
        discoveryTimer->stop();
    }

    // Send shutdown signal to all clients via UDP
    QString shutdownMsg = "SERVER_SHUTDOWN";
    udpSocket->writeDatagram(
        shutdownMsg.toUtf8(),
        QHostAddress::Broadcast,
        PORT
        );

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
                socket->write("kicked"); //sends msg in tcpsocket and checked in mainwindow::onDisconnectedFromServer
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
    loginwindow = new Login_Window();
    loginwindow->show();
    close();
}

