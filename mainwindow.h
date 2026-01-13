#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTextEdit>
#include <QMap>
#include <QHostAddress>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QSystemTrayIcon;
class Login_Window;

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
    void onTabChanged(int index);
    void onActivelistChanged(const QString& username);
    void receivePresenceAnnouncement();
    void announceDeparture();
    void on_logout_clicked();
    void closeEvent(QCloseEvent *event) override; //for when window directly closed so the activelist updates

    void loadChatHistory();
    void saveChatHistory();
    void loadChatHistoryForTab(const QString& tabname);
    void appendAlignedMessage(QTextEdit *view, const QString &text, Qt::Alignment alignment);
    void clearCurrentChatHistory();
    void setemojiBtn();

    //file send
    void on_attachFile_clicked();




private:
    Ui::MainWindow *ui;
    Login_Window *loginwindow;
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

    QTimer* announcementTimer ;
    bool isLoggingOut;

    QMenu *emojiMenu;

    //encrpyt chat history helper funcs
    static QByteArray getKey();
    static QString encryptString(const QString &plain);
    static QString decryptString(const QString &encrypted);

    //file attachment states(before sending)
    QString pendingFilePath;
    bool hasattachedFile;
    bool isSendingFile;
    QString currentTransferId;
    QString pendingTransferId;

    struct IncomingFile{
        QFile *file;
        QString fileName;
        QString sender;
        qint64 totalSize;
        qint64 bytesReceived;
        QString transferId;
    };

    QMap<QString, IncomingFile> activeDownloads; //qmap for transferid,file structure

    //hepler funcs- for file transfer
    void sendFile(const QString &recipient, const QString &caption);
    void handleFileTransferBlocked(const QString &reason);
    void handleFileTransferStart(const QString &transferId, const QString &sender,
                                 const QString &fileName, qint64 fileSize, const QString &caption);
    void handleFileData(const QString &transferId, const QByteArray &data);
    void handleFileComplete(const QString &transferId);
    void handleFileCancelled(const QString &transferId);
};

#endif // MAINWINDOW_H
