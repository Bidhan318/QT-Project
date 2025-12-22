#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTextEdit>
#include <QMap>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QSystemTrayIcon;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString username, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void receiveMessage();
    void announcePresence();
    void connectToServer();
    void onConnectedToServer();
    void onDisconnectedFromServer();
    void onTcpError(QAbstractSocket::SocketError error);

private:
    Ui::MainWindow *ui;

    // Username of logged in user
    QString LoggedUser;

    // Network sockets
    QUdpSocket *udpSocket;      // For sending/receiving messages (UDP broadcast)
    QTcpSocket *tcpSocket;      // For server presence detection

    // System tray for notifications
    QSystemTrayIcon *trayicon;

    // Chat management
    QMap<QString, QTextEdit*> chatTabs;           // Maps username to their chat tab
    QMap<QString, QHostAddress> activeClients;    // Maps username to their IP address

    // Helper functions
    void addChatTab(const QString &username);
    void removeChatTab(const QString &username);
    QTextEdit* getCurrentChatView();
};

#endif // MAINWINDOW_H
