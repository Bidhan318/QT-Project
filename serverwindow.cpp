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

    QByteArray data = clientSocket->readAll(); //reads all data from this client
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

    loginwindow = new Login_Window();
    loginwindow->show();
    close();
}

