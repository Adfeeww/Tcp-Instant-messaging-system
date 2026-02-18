#include "privatechat.h"
#include "ui_privatechat.h"
#include "protocol.h"
#include "tcpclient.h"
#include <QMessageBox>
#include <QDebug>

PrivateChat::PrivateChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
    ui->showMsg_te->setReadOnly(true);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

PrivateChat &PrivateChat::getInstance()
{
    static PrivateChat instance;
    return instance;
}

void PrivateChat::setChatName(QString name)
{
    m_strChatName = name;
    m_strLoginName = TcpClient::getInstance().LoginName();
}

void PrivateChat::updateMsg(const PDU *pdu)
{
    if (pdu == NULL){
        return;
    }

    char caSendName[32] = {'\0'};
    memcpy(caSendName, pdu->caData, 32);
    QString strMsg = QString("%1 says: %2").arg(caSendName).arg((char*)pdu->caMsg);
    ui->showMsg_te->append(strMsg);
}

void PrivateChat::on_sendMsg_pb_clicked()
{
    QString strMsg = ui->inputMsg_le->text();
    ui->inputMsg_le->clear();
    if (!strMsg.isEmpty()){
        PDU *pdu = mkPDU(strMsg.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;

        memcpy(pdu->caData, m_strLoginName.toStdString().c_str(), m_strLoginName.size());
        memcpy(pdu->caData + 32, m_strChatName.toStdString().c_str(), m_strChatName.size());
        memcpy(pdu->caMsg, strMsg.toStdString().c_str(), strMsg.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else {
        QMessageBox::warning(this, "警告", "消息不能为空");
    }
}
