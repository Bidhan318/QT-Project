/********************************************************************************
** Form generated from reading UI file 'login_window.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGIN_WINDOW_H
#define UI_LOGIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Login_Window
{
public:
    QWidget *centralwidget;
    QVBoxLayout *mainVerticalLayout;
    QSpacerItem *topSpacer;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *leftSpacer;
    QWidget *formContainer;
    QVBoxLayout *formLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLabel *label_2;
    QSpacerItem *titleSpacer;
    QLineEdit *Username;
    QLineEdit *Password;
    QSpacerItem *buttonSpacer;
    QHBoxLayout *loginButtonLayout;
    QSpacerItem *loginLeftSpacer;
    QPushButton *Login;
    QSpacerItem *loginRightSpacer;
    QHBoxLayout *registerButtonLayout;
    QSpacerItem *registerLeftSpacer;
    QPushButton *Register;
    QSpacerItem *registerRightSpacer;
    QSpacerItem *rightSpacer;
    QSpacerItem *bottomSpacer;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *Login_Window)
    {
        if (Login_Window->objectName().isEmpty())
            Login_Window->setObjectName("Login_Window");
        Login_Window->resize(800, 601);
        centralwidget = new QWidget(Login_Window);
        centralwidget->setObjectName("centralwidget");
        centralwidget->setStyleSheet(QString::fromUtf8("background-color: #D3D3D3;"));
        mainVerticalLayout = new QVBoxLayout(centralwidget);
        mainVerticalLayout->setObjectName("mainVerticalLayout");
        topSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        mainVerticalLayout->addItem(topSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        leftSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(leftSpacer);

        formContainer = new QWidget(centralwidget);
        formContainer->setObjectName("formContainer");
        formContainer->setMinimumSize(QSize(400, 0));
        formContainer->setMaximumSize(QSize(400, 16777215));
        formLayout = new QVBoxLayout(formContainer);
        formLayout->setSpacing(15);
        formLayout->setObjectName("formLayout");
        formLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_3 = new QLabel(formContainer);
        label_3->setObjectName("label_3");
        label_3->setMaximumSize(QSize(150, 150));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/images/novachat.png")));
        label_3->setScaledContents(true);

        horizontalLayout_3->addWidget(label_3);


        formLayout->addLayout(horizontalLayout_3);

        label_2 = new QLabel(formContainer);
        label_2->setObjectName("label_2");
        QFont font;
        font.setFamilies({QString::fromUtf8("Times New Roman")});
        font.setPointSize(16);
        font.setBold(true);
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        formLayout->addWidget(label_2);

        titleSpacer = new QSpacerItem(20, 30, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        formLayout->addItem(titleSpacer);

        Username = new QLineEdit(formContainer);
        Username->setObjectName("Username");
        Username->setMinimumSize(QSize(0, 35));
        Username->setStyleSheet(QString::fromUtf8("background-color: #C6FCFF;"));

        formLayout->addWidget(Username);

        Password = new QLineEdit(formContainer);
        Password->setObjectName("Password");
        Password->setMinimumSize(QSize(0, 35));
        Password->setStyleSheet(QString::fromUtf8("background-color: #C6FCFF;"));
        Password->setEchoMode(QLineEdit::EchoMode::Password);

        formLayout->addWidget(Password);

        buttonSpacer = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        formLayout->addItem(buttonSpacer);

        loginButtonLayout = new QHBoxLayout();
        loginButtonLayout->setObjectName("loginButtonLayout");
        loginLeftSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        loginButtonLayout->addItem(loginLeftSpacer);

        Login = new QPushButton(formContainer);
        Login->setObjectName("Login");
        Login->setMinimumSize(QSize(100, 35));
        Login->setMaximumSize(QSize(100, 16777215));
        Login->setStyleSheet(QString::fromUtf8("background-color: #C6FCFF;"));

        loginButtonLayout->addWidget(Login);

        loginRightSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        loginButtonLayout->addItem(loginRightSpacer);


        formLayout->addLayout(loginButtonLayout);

        registerButtonLayout = new QHBoxLayout();
        registerButtonLayout->setObjectName("registerButtonLayout");
        registerLeftSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        registerButtonLayout->addItem(registerLeftSpacer);

        Register = new QPushButton(formContainer);
        Register->setObjectName("Register");
        Register->setMinimumSize(QSize(100, 35));
        Register->setMaximumSize(QSize(100, 16777215));
        Register->setStyleSheet(QString::fromUtf8("background-color: #C6FCFF;"));

        registerButtonLayout->addWidget(Register);

        registerRightSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        registerButtonLayout->addItem(registerRightSpacer);


        formLayout->addLayout(registerButtonLayout);


        horizontalLayout->addWidget(formContainer);

        rightSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(rightSpacer);


        mainVerticalLayout->addLayout(horizontalLayout);

        bottomSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        mainVerticalLayout->addItem(bottomSpacer);

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
        label_3->setText(QString());
        label_2->setText(QCoreApplication::translate("Login_Window", "LOGIN", nullptr));
        Username->setPlaceholderText(QCoreApplication::translate("Login_Window", "Username", nullptr));
        Password->setPlaceholderText(QCoreApplication::translate("Login_Window", "Password", nullptr));
        Login->setText(QCoreApplication::translate("Login_Window", "Login", nullptr));
        Register->setText(QCoreApplication::translate("Login_Window", "Register", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Login_Window: public Ui_Login_Window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGIN_WINDOW_H
