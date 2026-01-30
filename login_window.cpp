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
#include <QUdpSocket>
#include <QEventLoop>
#include <QTimer>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QDir>
#include "port.h"

Login_Window::Login_Window(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::Login_Window),
    discoverySocket(nullptr),
    serverFound(false),
    userCancelled(false)
{
    ui->setupUi(this);

    connect(ui->Password, &QLineEdit::returnPressed,  //if enter pressed in password field it signals login funcs
            this, &Login_Window::on_Login_clicked);
    connect(ui->Username, &QLineEdit::returnPressed,
            ui->Password, QOverload<>::of(&QWidget::setFocus));

    // Initialize users.json if it doesn't exist
    QString usersPath = getUsersJsonPath();
    if (!QFile::exists(usersPath))
    {
        QFile file(usersPath);
        if (file.open(QIODevice::WriteOnly))
        {
            QJsonObject root;
            QJsonArray users;

            // Default admin (username: admin, password: admin1)
            QJsonObject admin;
            admin["username"] = "admin";
            admin["password"] = hashPassword("admin1");
            users.append(admin);

            root["users"] = users;
            file.write(QJsonDocument(root).toJson());
            file.close();
        }
    }

}

Login_Window::~Login_Window()
{
    if (discoverySocket) {
        discoverySocket->close();
        delete discoverySocket;
    }
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

    QFile file(getUsersJsonPath());
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
            if (isServerAlreadyRunning())
            {
                // Only show "server running" message if server was actually found
                if (serverFound && !userCancelled)
                {
                    QMessageBox::warning(this, "Server Already Running",
                                         "A server instance is already running on this network.\n"
                                         "Only one admin can host the server at a time.");
                }
                // If userCancelled is true, message was already shown
                return;
            }


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

   QFile file(getUsersJsonPath());
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

void Login_Window::checkForExistingServer()
{
    while (discoverySocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(discoverySocket->pendingDatagramSize());
        discoverySocket->readDatagram(datagram.data(), datagram.size());

        QString message = QString::fromUtf8(datagram);

        if (message.startsWith("SERVER_DISCOVERY:"))
        {
            serverFound = true;
        }
    }
}

bool Login_Window::isServerAlreadyRunning()
{
    if (!discoverySocket) {
        discoverySocket = new QUdpSocket(this);
        discoverySocket->bind(QHostAddress::AnyIPv4, PORT,
                              QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(discoverySocket, &QUdpSocket::readyRead,
                this, &Login_Window::checkForExistingServer);
    }

    serverFound = false;
    userCancelled = false;
    bool checkcomplete = false;

    // Create progress dialog
    QProgressDialog progress("Checking for existing server on network...",
                             "Cancel", 0, 25, this);
    progress.setWindowTitle("Novachat-Server Discovery");
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);  // Show immediately
    progress.setAutoClose(true);
    progress.setAutoReset(false);

    // Update progress bar while waiting
    QEventLoop loop;
    QTimer progressTimer;
    int progressValue = 0;

    connect(&progressTimer, &QTimer::timeout, [&]() {
        progressValue++;
        progress.setValue(progressValue);

        if (progressValue >= 25 || serverFound) {
            checkcomplete = true;
            loop.quit(); //exit the waiting loop
        }
    });

    connect(&progress, &QProgressDialog::canceled, [&]() {
        if(!checkcomplete)
        {
            userCancelled = true;
        }
        loop.quit();
    });

    progressTimer.start(100);  // Update every 100ms (25 * 100ms = 2.5 seconds)
    loop.exec();

    progressTimer.stop();
    progress.close();
    if (userCancelled) {
        QMessageBox::information(this, "Check Cancelled",
                                 "Server check was cancelled.\n"
                                 "For safety, admin login is blocked.\n"
                                 "Please try again and wait for the check to complete.");
        return true;  // Block login to be safe
    }

    return serverFound;
}

QString Login_Window::getUsersJsonPath()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/users.json";
}
