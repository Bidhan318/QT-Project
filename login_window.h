#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include <QMainWindow>

class MainWindow;

QT_BEGIN_NAMESPACE
namespace Ui {
class Login_Window;
}
QT_END_NAMESPACE

class Login_Window : public QMainWindow
{
    Q_OBJECT

public:
    Login_Window(QWidget *parent = nullptr);
    ~Login_Window();

private slots:
    void on_Login_clicked();

    void on_Register_clicked();

private:
    Ui::Login_Window *ui;
    MainWindow* mainwindow;
};
#endif // LOGIN_WINDOW_H
