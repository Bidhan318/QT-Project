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
    QLabel *label;
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
        Login_Window->resize(800, 600);
        centralwidget = new QWidget(Login_Window);
        centralwidget->setObjectName("centralwidget");
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
        label = new QLabel(formContainer);
        label->setObjectName("label");
        QFont font;
        font.setFamilies({QString::fromUtf8("Times New Roman")});
        font.setPointSize(16);
        font.setBold(true);
        label->setFont(font);
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        formLayout->addWidget(label);

        label_2 = new QLabel(formContainer);
        label_2->setObjectName("label_2");
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        formLayout->addWidget(label_2);

        titleSpacer = new QSpacerItem(20, 30, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        formLayout->addItem(titleSpacer);

        Username = new QLineEdit(formContainer);
        Username->setObjectName("Username");
        Username->setMinimumSize(QSize(0, 35));

        formLayout->addWidget(Username);

        Password = new QLineEdit(formContainer);
        Password->setObjectName("Password");
        Password->setMinimumSize(QSize(0, 35));
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
        label->setText(QCoreApplication::translate("Login_Window", "NOVACHAT", nullptr));
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
