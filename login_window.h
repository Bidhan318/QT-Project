#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include <QMainWindow>
#include <QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
class Login_Window;
}
QT_END_NAMESPACE

class MainWindow;
class ServerWindow;

class Login_Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Login_Window(QWidget *parent = nullptr);
    ~Login_Window();

private slots:
    void on_Login_clicked();
    void on_Register_clicked();

private:
    Ui::Login_Window *ui;
    MainWindow *mainwindow;
    ServerWindow *serverwindow;

    // Hashing function (SHA-256)
    QString hashPassword(const QString &password);
    QUdpSocket *discoverySocket;
    bool serverFound;
    bool userCancelled;
    void checkForExistingServer();
    bool isServerAlreadyRunning(); //to prevent multiple admin login
};

#endif // LOGIN_WINDOW_H
