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
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
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
    QTextEdit *textEdit;
    QTextEdit *textEdit_2;
    QPushButton *pushButton;
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
        textEdit = new QTextEdit(widget);
        textEdit->setObjectName("textEdit");
        textEdit->setGeometry(QRect(210, 290, 331, 31));
        textEdit->viewport()->setProperty("cursor", QVariant(QCursor(Qt::CursorShape::ArrowCursor)));
        textEdit_2 = new QTextEdit(widget);
        textEdit_2->setObjectName("textEdit_2");
        textEdit_2->setGeometry(QRect(210, 330, 331, 31));
        pushButton = new QPushButton(widget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(340, 390, 90, 29));

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
        textEdit->setPlaceholderText(QCoreApplication::translate("Login_Window", "Username", nullptr));
        textEdit_2->setPlaceholderText(QCoreApplication::translate("Login_Window", "Password", nullptr));
        pushButton->setText(QCoreApplication::translate("Login_Window", "Login", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Login_Window: public Ui_Login_Window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGIN_WINDOW_H
