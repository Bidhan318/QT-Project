/********************************************************************************
** Form generated from reading UI file 'serverwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SERVERWINDOW_H
#define UI_SERVERWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ServerWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *mainLayout;
    QFrame *chatFrame;
    QVBoxLayout *chatLayout;
    QLabel *label;
    QTabWidget *client_tab;
    QHBoxLayout *hboxLayout;
    QLabel *msg_lbl;
    QLineEdit *msg_in;
    QPushButton *send_btn;
    QFrame *clientsFrame;
    QVBoxLayout *clientLayout;
    QLabel *label_3;
    QTextEdit *client_name;
    QPushButton *disconnect_btn;

    void setupUi(QMainWindow *ServerWindow)
    {
        if (ServerWindow->objectName().isEmpty())
            ServerWindow->setObjectName("ServerWindow");
        ServerWindow->resize(900, 600);
        centralwidget = new QWidget(ServerWindow);
        centralwidget->setObjectName("centralwidget");
        mainLayout = new QHBoxLayout(centralwidget);
        mainLayout->setSpacing(6);
        mainLayout->setObjectName("mainLayout");
        chatFrame = new QFrame(centralwidget);
        chatFrame->setObjectName("chatFrame");
        chatFrame->setFrameShape(QFrame::Shape::StyledPanel);
        chatFrame->setFrameShadow(QFrame::Shadow::Plain);
        chatLayout = new QVBoxLayout(chatFrame);
        chatLayout->setObjectName("chatLayout");
        label = new QLabel(chatFrame);
        label->setObjectName("label");
        QFont font;
        font.setFamilies({QString::fromUtf8("Times New Roman")});
        font.setPointSize(26);
        label->setFont(font);

        chatLayout->addWidget(label);

        client_tab = new QTabWidget(chatFrame);
        client_tab->setObjectName("client_tab");

        chatLayout->addWidget(client_tab);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        msg_lbl = new QLabel(chatFrame);
        msg_lbl->setObjectName("msg_lbl");

        hboxLayout->addWidget(msg_lbl);

        msg_in = new QLineEdit(chatFrame);
        msg_in->setObjectName("msg_in");
        msg_in->setAutoFillBackground(false);

        hboxLayout->addWidget(msg_in);

        send_btn = new QPushButton(chatFrame);
        send_btn->setObjectName("send_btn");

        hboxLayout->addWidget(send_btn);


        chatLayout->addLayout(hboxLayout);


        mainLayout->addWidget(chatFrame);

        clientsFrame = new QFrame(centralwidget);
        clientsFrame->setObjectName("clientsFrame");
        clientsFrame->setFrameShape(QFrame::Shape::StyledPanel);
        clientsFrame->setFrameShadow(QFrame::Shadow::Plain);
        clientLayout = new QVBoxLayout(clientsFrame);
        clientLayout->setObjectName("clientLayout");
        label_3 = new QLabel(clientsFrame);
        label_3->setObjectName("label_3");
        label_3->setFont(font);

        clientLayout->addWidget(label_3);

        client_name = new QTextEdit(clientsFrame);
        client_name->setObjectName("client_name");
        client_name->setReadOnly(true);

        clientLayout->addWidget(client_name);

        disconnect_btn = new QPushButton(clientsFrame);
        disconnect_btn->setObjectName("disconnect_btn");

        clientLayout->addWidget(disconnect_btn);


        mainLayout->addWidget(clientsFrame);

        mainLayout->setStretch(0, 4);
        mainLayout->setStretch(1, 2);
        ServerWindow->setCentralWidget(centralwidget);

        retranslateUi(ServerWindow);

        QMetaObject::connectSlotsByName(ServerWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ServerWindow)
    {
        chatFrame->setStyleSheet(QCoreApplication::translate("ServerWindow", "\n"
"        QFrame#chatFrame {\n"
"          background-color: #f0f0f0;\n"
"          border: 1px solid #c8c8c8;\n"
"          border-radius: 4px;\n"
"        }\n"
"       ", nullptr));
        label->setText(QCoreApplication::translate("ServerWindow", "Chats", nullptr));
        msg_lbl->setText(QCoreApplication::translate("ServerWindow", "Message:", nullptr));
        send_btn->setText(QCoreApplication::translate("ServerWindow", "Send", nullptr));
        clientsFrame->setStyleSheet(QCoreApplication::translate("ServerWindow", "\n"
"        QFrame#clientsFrame {\n"
"          background-color: #f5f5f5;\n"
"          border: 1px solid #c8c8c8;\n"
"          border-radius: 4px;\n"
"        }\n"
"       ", nullptr));
        label_3->setText(QCoreApplication::translate("ServerWindow", "Clients", nullptr));
        client_name->setPlaceholderText(QCoreApplication::translate("ServerWindow", "Client list here...", nullptr));
        disconnect_btn->setText(QCoreApplication::translate("ServerWindow", "Disconnect all", nullptr));
        (void)ServerWindow;
    } // retranslateUi

};

namespace Ui {
    class ServerWindow: public Ui_ServerWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SERVERWINDOW_H
