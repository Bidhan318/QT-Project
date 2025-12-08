#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString username,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->displaylabel->setText("Welcome to the software, " + username);
}

MainWindow::~MainWindow()
{
    delete ui;
}
