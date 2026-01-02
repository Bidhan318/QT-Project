#include "login_window.h"
#include "mainwindow.h"
#include "serverwindow.h"
#include "ui_login_window.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QDebug>

Login_Window::Login_Window(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::Login_Window)
{
    ui->setupUi(this);

    connect(ui->Password, &QLineEdit::returnPressed,  //if enter pressed in password field it signals login funcs
            this, &Login_Window::on_Login_clicked);
    connect(ui->Username, &QLineEdit::returnPressed,
            ui->Password, QOverload<>::of(&QWidget::setFocus));

}

Login_Window::~Login_Window()
{
    delete ui;
}

/* Converts plain text password into SHA-256 hash */
QString Login_Window::hashPassword(const QString &password)
{
    return QCryptographicHash::hash(
               password.toUtf8(),
               QCryptographicHash::Sha256
               ).toHex();
}

/* ================= LOGIN ================= */
void Login_Window::on_Login_clicked()
{
    QString username = ui->Username->text();
    QString password = ui->Password->text();

    // Hash entered password before comparing
    QString hashedPassword = hashPassword(password);

    QFile file("users.json");
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Could not open users.json");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
    {
        QMessageBox::critical(this, "Error", "Invalid JSON format");
        return;
    }

    QJsonArray users = doc.object()["users"].toArray();
    bool success = false;

    // Loop through users array
    for (int i = 0; i < users.size(); ++i)
    {
        QJsonObject user = users[i].toObject();

        // Compare username and hashed password
        if (user["username"].toString() == username &&
            user["password"].toString() == hashedPassword)
        {
            success = true;
            break;
        }
    }

    if (success)
    {
        // Admin check - open ServerWindow for admin, MainWindow for others
        if (username.toLower() == "admin")
        {
            // Open ServerWindow for admin
            serverwindow = new ServerWindow(username);
            serverwindow->setWindowTitle("NovaChat Server - " + username);
            serverwindow->show();
            close();
        }
        else
        {
            // Open MainWindow (Client) for regular users
            mainwindow = new MainWindow(username);
            mainwindow->setWindowTitle("NovaChat - " + username);
            mainwindow->show();
            close();
        }
    }
    else
    {
        QMessageBox::warning(this, "Login Failed",
                             "Username or password is incorrect");
    }
}

/* ================= REGISTER ================= */
void Login_Window::on_Register_clicked()
{
    QString username = ui->Username->text();
    QString password = ui->Password->text();

    if (username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "Error",
                             "Username and password cannot be empty");
        return;
    }

    QFile file("users.json");
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Could not open users.json");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root;
    QJsonArray users;

    if (doc.isObject())
    {
        root = doc.object();
        users = root["users"].toArray();
    }

    // Check if username already exists
    for (int i = 0; i < users.size(); ++i)
    {
        if (users[i].toObject()["username"].toString() == username)
        {
            QMessageBox::warning(this, "Error",
                                 "Username already exists");
            return;
        }
    }

    // Create new user object
    QJsonObject newUser;
    newUser["username"] = username;

    // Store hashed password (NOT plain text)
    newUser["password"] = hashPassword(password);

    users.append(newUser);
    root["users"] = users;

    // Write updated JSON back to file
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Could not write users.json");
        return;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();

    QMessageBox::information(this, "Success",
                             "User registered successfully");
}
