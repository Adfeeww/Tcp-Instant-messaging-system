#include "tcpclient.h"
#include "ui_tcpclient.h"

#include <QDebug>
#include <QByteArray>
#include <QMessageBox>
#include <QSettings>
#include <QHostAddress>
#include "protocol.h"
#include "opewidget.h"
#include "online.h"
#include "friend.h"
#include "privatechat.h"
#include "book.h"

TcpClient::TcpClient(QWidget *parent) : QWidget(parent), ui(new Ui::TcpClient)
{
    ui->setupUi(this);

    resize(500, 250);

    loadConfig();

    connect(&m_tcpSocket, &QTcpSocket::connected, this, &TcpClient::showConnect);
    connect(&m_tcpSocket, SIGNAL(readyRead()), this, SLOT(recvMsg()));

    //连接服务器
    m_tcpSocket.connectToHost(QHostAddress(m_strIP), m_usPort);
}

TcpClient::~TcpClient()
{

    delete ui;
}

//加载配置(IP/Port)
void TcpClient::loadConfig()
{
    QSettings settings("client.ini", QSettings::IniFormat);

    //文件不存在则配置文件
    if (!settings.contains("network/ip")){
        settings.setValue("network/ip", "127.0.0.1");
        settings.setValue("network/port", 8888);
        settings.sync();
    }

    //读取文件
    m_strIP = settings.value("network/ip").toString();

    bool ok = false;
    m_usPort = settings.value("network/port").toString().toUInt(&ok);

    if (!ok || m_usPort == 0){
        QMessageBox::critical(this, "config error", "invalid port");
    }
}

TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()
{
    return m_tcpSocket;
}

QString TcpClient::LoginName()
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this, "连接服务器", "连接服务器成功");
}

void TcpClient::recvMsg()
{
    if (!OpeWidget::getInstance().getBook()->getDownloadStatus()){
        uint uiPDULen = 0;
        m_tcpSocket.read((char*)&uiPDULen, sizeof(uint));
        uint uiMsgLen = uiPDULen - sizeof (PDU);
        PDU* pdu = mkPDU(uiMsgLen);
        m_tcpSocket.read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint));
        switch (pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_RESPOND:
        {
            if (strcmp(pdu->caData, REGIST_OK) == 0){
                QMessageBox::information(this, "注册", "注册成功");
            }
            else if (strcmp(pdu->caData, REGIST_FAILED) == 0){
                QMessageBox::warning(this, "警告", "注册失败");
            }
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_RESPOND:
        {
            if (strcmp(pdu->caData, LOGIN_OK) == 0){
                m_strCurPath = QString("./%1").arg(m_strLoginName);
                QMessageBox::information(this, "登录", "登录成功");
                OpeWidget::getInstance().show();
                this->hide();
            }
            else if (strcmp(pdu->caData, LOGIN_FAILED) == 0){
                QMessageBox::warning(this, "警告", "登录失败");
            }
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:
        {
            OpeWidget::getInstance().getFriend()->showAllOnlineUsr(pdu);
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:
        {
            if (strcmp(SEARCH_USR_NO, pdu->caData) == 0){
                QMessageBox::information(this, "搜索", "no such person");
            }
            else if (strcmp(SEARCH_USR_ONLINE, pdu->caData) == 0){
                QMessageBox::information(this, "搜索", "online");
            }
            else if (strcmp(SEARCH_USR_OFFLINE, pdu->caData) == 0){
                QMessageBox::information(this, "搜索", "offline");
            }
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData + 32, 32);
            int ret = QMessageBox::information(this, "添加好友", QString("%1 want to add you as friend ?").arg(caName), QMessageBox::Yes, QMessageBox::No);
            PDU *respdu = mkPDU(0);
            memcpy(respdu->caData, caName, 32);
            if (ret == QMessageBox::Yes){
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE;
            }
            else if (ret == QMessageBox::No){
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
            }
            m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:
        {
            QMessageBox::information(this, "添加好友", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:
        {
            OpeWidget::getInstance().getFriend()->updateFriendList(pdu);
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND:
        {
            QMessageBox::information(this, "删除好友", "删除好友成功");
            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            if (PrivateChat::getInstance().isHidden()){
                PrivateChat::getInstance().show();
            }
            char caSendName[32] = {'\0'};
            memcpy(caSendName, pdu->caData, 32);
            QString strSendName = caSendName;
            PrivateChat::getInstance().setChatName(strSendName);
            PrivateChat::getInstance().show();
            PrivateChat::getInstance().updateMsg(pdu);
            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            OpeWidget::getInstance().getFriend()->updateGroupChat(pdu);
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:
        {
            QMessageBox::information(this, "创建文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:
        {
            OpeWidget::getInstance().getBook()->updateFileList(pdu);
            break;
        }
        case ENUM_MSG_TYPE_DELETE_DIR_OK:
        {
            QMessageBox::information(this, "信息", "删除成功");
            break;
        }
        case ENUM_MSG_TYPE_DELETE_DIR_FAILED:
        {
            QMessageBox::information(this, "信息", "删除失败");
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_RESPOND:
        {
            QMessageBox::information(this, "提示", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:
        {
            QMessageBox::information(this, "上传文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FILE_RESPOND:
        {
            QMessageBox::information(this, "删除文件", pdu->caData);
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:
        {
            char fileName[32] = {'\0'};
            qint64 saveFileSize = 0;
            sscanf(pdu->caData, "%s %lld", fileName, &saveFileSize);

            QString m_strSaveFilePath = OpeWidget::getInstance().getBook()->getSaveFilePath();
            m_file.setFileName(m_strSaveFilePath);
            if (m_file.open(QIODevice::WriteOnly)){
                OpeWidget::getInstance().getBook()->setDownloadStatus(true);
                OpeWidget::getInstance().getBook()->m_iTotal = saveFileSize;
                OpeWidget::getInstance().getBook()->m_iRecved = 0;
            }
            else {
                QMessageBox::critical(this, "错误", "打开文件失败");
            }

            break;
        }
        default:
            break;
        }

        free(pdu);
        pdu = NULL;
    }
    else {
        QByteArray buffer = m_tcpSocket.readAll();
        m_file.write(buffer);

        Book *pBook = OpeWidget::getInstance().getBook();
        pBook->m_iRecved += buffer.size();
        if (pBook->m_iTotal == pBook->m_iRecved){
            QMessageBox::information(this, "下载文件", "下载文件成功");
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);
        }
        else if (pBook->m_iTotal < pBook->m_iRecved){
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::critical(this, "下载文件", "下载文件失败");
        }
    }
}

void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();

    if (!strName.isEmpty() && !strPwd.isEmpty()){
        m_strLoginName = strName;
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;
        strncpy(pdu->caData, strName.toStdString().c_str(), 32);
        strncpy(pdu->caData + 32, strPwd.toStdString().c_str(), 32);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else {
        QMessageBox::warning(this, "警告", "登录失败");
    }
}

void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();
    if (!strName.isEmpty() && !strPwd.isEmpty()){
        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;
        strncpy(pdu->caData, strName.toStdString().c_str(), 32);
        strncpy(pdu->caData + 32, strPwd.toStdString().c_str(), 32);
        m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else {
        QMessageBox::warning(this, "警告", "注册失败");
    }
}

void TcpClient::on_logout_pb_clicked()
{

}
