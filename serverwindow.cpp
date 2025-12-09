#include "serverwindow.h"
#include "ui_serverwindow.h"

ServerWindow::ServerWindow(QString username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ServerWindow)
{
    ui->setupUi(this);
    Loggeduser = username;
    ui->label->setText("Welcome to the software, " + username);
}

ServerWindow::~ServerWindow()
{
    delete ui;
}
