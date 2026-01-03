#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "login_window.h"
#include <QMainWindow>
#include <QTextEdit>
#include <QMap>
#include <QHostAddress>
#include <QSystemTrayIcon>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString username, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectToServer();
    void sendMessage();
    void receiveMessage();
    void receivePresenceAnnouncement();

    void onConnectedToServer();
    void onDisconnectedFromServer();
    void onTcpError(QAbstractSocket::SocketError error);

    void onTabChanged(int index);
    void onActivelistChanged(const QString &username);
    void on_logout_clicked();

private:
    // UI
    Ui::MainWindow *ui;
    Login_Window *loginwindow;
    // user
    QString LoggedUser;

    // networking
    QTcpSocket *tcpSocket;
    QUdpSocket *udpSocket;
    QTimer *announcementTimer;

    // tray
    QSystemTrayIcon *trayicon;

    // chat data
    QMap<QString, QTextEdit*> chatTabs;
    QMap<QString, QHostAddress> activeClients;

    // helpers
    void addChatTab(const QString &username);
    void removeChatTab(const QString &username);
    QTextEdit* getCurrentChatView();

    void announcePresence();
    void announceDeparture();

    void loadChatHistory();
    void saveChatHistory();
    void appendAlignedMessage(QTextEdit *view, const QString &text, Qt::Alignment alignment);


};

#endif // MAINWINDOW_H
