#include "tcpserver.h"
#include "ui_tcpserver.h"
#include "mytcpserver.h"
#include <QMessageBox>
#include <QSettings>

TcpServer::TcpServer(QWidget *parent) : QWidget(parent) , ui(new Ui::TcpServer)
{
    ui->setupUi(this);

    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP), m_usPort);
}

TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::loadConfig()
{
    QSettings settings("server.ini", QSettings::IniFormat);

    if (!settings.contains("network/ip")){
        settings.setValue("network/ip", "127.0.0.1");
        settings.setValue("network/port", 8888);
        settings.sync();
    }

    m_strIP = settings.value("network/ip").toString();
    m_usPort = settings.value("network/port").toString().toUInt();
}
