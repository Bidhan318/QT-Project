#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QMap>
#include <QSet> //qset is a collection where no duplicates allowed

QT_BEGIN_NAMESPACE
namespace Ui {
class ServerWindow;
}
QT_END_NAMESPACE

class Login_Window;
class QTimer;

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ServerWindow(QString username, QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void on_send_btn_clicked();
    void on_disconnect_btn_clicked();
    void onNewConnection();
    void onClientDataReceived();
    void onClientDisconnected();
    void broadcastServerPresence();
    void onTabscloseRequested(int index);
    void on_logout_clicked();

private:
    Ui::ServerWindow *ui;
    Login_Window *loginwindow;
    // Server username
    QString Loggeduser;

    // Network components
    QUdpSocket *udpSocket;                        // For broadcasting messages
    QTcpServer *tcpServer;                        // For accepting client connections
    QTimer *discoveryTimer;                       // Timer for periodic server presence broadcast

    // Client management
    QMap<QTcpSocket*, QString> clientSockets;     // Maps TCP socket to username
    QMap<QString, QTextEdit*> clientTabs;         // Maps username to their chat tab

    // Helper functions
    void addClientTab(const QString &username);
    void removeClientTab(const QString &username);

    //file transfer
    struct FileTransferInfo{
        QString sender;
        QString recipient;
        QString fileName;
        qint64 fileSize;
        qint64 bytesTransferred;
        QString transferId;
        qint64 expectedSize;   // total file size

    };
    QMap<QString, FileTransferInfo> activeTransfers;  // transferId -> transfer info
    QMap<QTcpSocket*, QString> socketToTransferId;    // socket -> transferId
    QSet<QString> busyRecipients;                     // Users currently receiving files
    QSet<QString> busySenders;                        // Users currently sending files

    //file transfer hepler funcs
    QString generateTransferId(const QString &sender, const QString &recipient);
    bool canStartTransfer(const QString &sender, const QString &recipient);
    void handleFileDataRelay(QTcpSocket *senderSocket, const QString &transferId);
    void cleanupUserTransfers(const QString &username);
};


#endif // SERVERWINDOW_H
