/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QSpacerItem *horizontalSpacer;
    QPushButton *clearChatBtn_;
    QPushButton *logout;
    QTabWidget *chatTabs;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *activelist;
    QLabel *label_2;
    QLineEdit *messageEdit;
    QPushButton *sendBtn;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setMaximumSize(QSize(50, 50));
        label_4->setPixmap(QPixmap(QString::fromUtf8(":/images/novachat.png")));
        label_4->setScaledContents(true);

        horizontalLayout_2->addWidget(label_4);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        clearChatBtn_ = new QPushButton(centralwidget);
        clearChatBtn_->setObjectName("clearChatBtn_");

        horizontalLayout_2->addWidget(clearChatBtn_);

        logout = new QPushButton(centralwidget);
        logout->setObjectName("logout");
        logout->setStyleSheet(QString::fromUtf8("background-color: #EF5350;"));

        horizontalLayout_2->addWidget(logout);


        verticalLayout->addLayout(horizontalLayout_2);

        chatTabs = new QTabWidget(centralwidget);
        chatTabs->setObjectName("chatTabs");
        chatTabs->setTabsClosable(false);

        verticalLayout->addWidget(chatTabs);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(centralwidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        activelist = new QComboBox(centralwidget);
        activelist->setObjectName("activelist");
        activelist->setMinimumSize(QSize(150, 0));

        horizontalLayout->addWidget(activelist);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");

        horizontalLayout->addWidget(label_2);

        messageEdit = new QLineEdit(centralwidget);
        messageEdit->setObjectName("messageEdit");

        horizontalLayout->addWidget(messageEdit);

        sendBtn = new QPushButton(centralwidget);
        sendBtn->setObjectName("sendBtn");

        horizontalLayout->addWidget(sendBtn);


        verticalLayout->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        chatTabs->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "NovaChat", nullptr));
        label_4->setText(QString());
        clearChatBtn_->setText(QCoreApplication::translate("MainWindow", "ClearChat", nullptr));
        logout->setText(QCoreApplication::translate("MainWindow", "Logout", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "To:", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Message:", nullptr));
        messageEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "Type your message...", nullptr));
        sendBtn->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
