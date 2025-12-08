/********************************************************************************
** Form generated from reading UI file 'login_window.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGIN_WINDOW_H
#define UI_LOGIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Login_Window
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QWidget *widget;
    QLabel *label;
    QLabel *label_2;
    QPushButton *Login;
    QPushButton *Register;
    QLineEdit *Password;
    QLineEdit *Username;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *Login_Window)
    {
        if (Login_Window->objectName().isEmpty())
            Login_Window->setObjectName("Login_Window");
        Login_Window->resize(800, 600);
        Login_Window->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        Login_Window->setAutoFillBackground(false);
        centralwidget = new QWidget(Login_Window);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        widget = new QWidget(centralwidget);
        widget->setObjectName("widget");
        widget->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        label = new QLabel(widget);
        label->setObjectName("label");
        label->setGeometry(QRect(270, 120, 221, 81));
        QFont font;
        font.setFamilies({QString::fromUtf8("Times New Roman")});
        font.setPointSize(16);
        font.setBold(true);
        label->setFont(font);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        label_2 = new QLabel(widget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(270, 160, 221, 81));
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        Login = new QPushButton(widget);
        Login->setObjectName("Login");
        Login->setGeometry(QRect(340, 380, 90, 29));
        Register = new QPushButton(widget);
        Register->setObjectName("Register");
        Register->setGeometry(QRect(340, 420, 90, 29));
        Password = new QLineEdit(widget);
        Password->setObjectName("Password");
        Password->setGeometry(QRect(210, 330, 331, 28));
        Password->setEchoMode(QLineEdit::EchoMode::Password);
        Username = new QLineEdit(widget);
        Username->setObjectName("Username");
        Username->setGeometry(QRect(210, 290, 331, 28));

        gridLayout->addWidget(widget, 0, 0, 1, 1);

        Login_Window->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(Login_Window);
        statusbar->setObjectName("statusbar");
        Login_Window->setStatusBar(statusbar);

        retranslateUi(Login_Window);

        QMetaObject::connectSlotsByName(Login_Window);
    } // setupUi

    void retranslateUi(QMainWindow *Login_Window)
    {
        Login_Window->setWindowTitle(QCoreApplication::translate("Login_Window", "Login_Window", nullptr));
        label->setText(QCoreApplication::translate("Login_Window", "NOVACHAT", nullptr));
        label_2->setText(QCoreApplication::translate("Login_Window", "LOGIN", nullptr));
        Login->setText(QCoreApplication::translate("Login_Window", "Login", nullptr));
        Register->setText(QCoreApplication::translate("Login_Window", "Register", nullptr));
        Password->setPlaceholderText(QCoreApplication::translate("Login_Window", "Password", nullptr));
        Username->setPlaceholderText(QCoreApplication::translate("Login_Window", "Username", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Login_Window: public Ui_Login_Window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGIN_WINDOW_H
