#include "login_window.h"
#include "./ui_login_window.h"

Login_Window::Login_Window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login_Window)
{
    ui->setupUi(this);
}

Login_Window::~Login_Window()
{
    delete ui;
}
