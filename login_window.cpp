#include "login_window.h"
#include "mainwindow.h"
#include "./ui_login_window.h"
#include <QString>
#include <QMessageBox>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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



void Login_Window::on_Login_clicked()
{
    QString username = ui->Username->text();
    QString password = ui->Password->text();


    // Open JSON file
    QFile file("users.json"); // users.json is in the build folder of the project
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Could not open user data file");
        return;
    }

    QByteArray data = file.readAll(); //stores all data of json into an array format
    file.close();

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);  //qt understands it as an actual structured json doc
    if(!doc.isObject())
    {
        QMessageBox::critical(this, "Error", "Invalid JSON format");
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray usersArray = rootObj["users"].toArray();    //qjson array sores like this: { "username": "abc", "password": "123" },

    bool loginSuccess = false;



    for (int i = 0; i < usersArray.size(); ++i)
    {
        QJsonObject userObj = usersArray[i].toObject();  //toobject converts into an usable qobject for checking
        QString u = userObj["username"].toString();
        QString p = userObj["password"].toString();

        if(username == u && password == p)
        {
            loginSuccess = true;
            break;
        }
    }



    if(loginSuccess)
    {
        mainwindow = new MainWindow(username);
        mainwindow->show();
        this->close();
    }
    else
    {
        QMessageBox::critical(this,"Login Failed","Username or Password is incorrect");
    }
}



void Login_Window::on_Register_clicked()
{
    QString username = ui->Username->text();
    QString password = ui->Password->text();

    if(username.isEmpty() || password.isEmpty())
    {
        QMessageBox::warning(this, "Input Error", "Username and password cannot be empty");
        return;
    }

    QFile file("users.json");
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Could not open user data file for reading");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(!doc.isObject())
    {
        QMessageBox::critical(this, "Error", "Invalid JSON format");
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray usersArray = rootObj["users"].toArray();

    // Check if username already exists
    for(int i = 0; i < usersArray.size(); ++i)
    {
        QJsonObject userObj = usersArray[i].toObject();
        if(userObj["username"].toString() == username)
        {
            QMessageBox::warning(this, "Registration Failed", "Username already exists");
            return;
        }
    }

    // Add new user
    QJsonObject newUser;
    newUser["username"] = username;
    newUser["password"] = password; // Plain text for now
    usersArray.append(newUser);

    // Update JSON object
    rootObj["users"] = usersArray;
    QJsonDocument newDoc(rootObj);

    // Write back to file
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, "Error", "Could not open user data file for writing");
        return;
    }

    file.write(newDoc.toJson());
    file.close();

    QMessageBox::information(this, "Success", "User registered successfully");
}


