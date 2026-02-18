#include "friend.h"
#include "online.h"
#include "tcpclient.h"
#include "protocol.h"
#include "privatechat.h"
#include <QDebug>
#include <QMessageBox>

Friend::Friend(QWidget *parent) : QWidget(parent)
{
    m_pShowMsgTE = new QTextEdit;
    m_pFriendListWidget = new QListWidget;
    m_pInputMsgLE = new QLineEdit;

    m_pShowMsgTE->setReadOnly(true);

    m_pDelFriendPB = new QPushButton("删除好友");
    m_pFlushFriendPB = new QPushButton("刷新好友");
    m_pShowOnlineUsrPB = new QPushButton("显示在线人数");
    m_pSearchPB = new QPushButton("查找用户");
    m_pMsgSendPB = new QPushButton("信息发送");
    m_pPrivateChatPB = new QPushButton("私聊");

    QVBoxLayout *pRightPBVBL = new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUsrPB);
    pRightPBVBL->addWidget(m_pSearchPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);

    QHBoxLayout *pTopHBL = new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);

    QHBoxLayout *pMsgHBL = new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);

    m_pOnline = new Online;

    QVBoxLayout *pMain = new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);
    pMain->addWidget(m_pOnline);
    m_pOnline->hide();

    setLayout(pMain);

    connect(m_pShowOnlineUsrPB, SIGNAL(clicked(bool)), this, SLOT(showOnline()));
    connect(m_pSearchPB, SIGNAL(clicked(bool)), this, SLOT(searchUsr()));
    connect(m_pFlushFriendPB, SIGNAL(clicked(bool)), this, SLOT(flushFriend()));
    connect(m_pDelFriendPB, SIGNAL(clicked(bool)), this, SLOT(delFriend()));
    connect(m_pPrivateChatPB, SIGNAL(clicked(bool)), this, SLOT(privateChat()));
    connect(m_pMsgSendPB, SIGNAL(clicked(bool)), this, SLOT(groupChat()));
}

void Friend::showAllOnlineUsr(PDU *pdu)
{
    if (pdu == NULL){
        return;
    }

    m_pOnline->showUsr(pdu);
}

void Friend::updateFriendList(PDU *pdu)
{
    if (pdu == NULL){
        return;
    }

    m_pFriendListWidget->clear();

    uint uiSize = pdu->uiMsgLen / 32;
    char caName[32] = {'\0'};
    for (uint i = 0; i < uiSize; i ++){
        memcpy(caName, (char*)pdu->caMsg + i * 32, 32);
        m_pFriendListWidget->addItem(caName);
    }
}

void Friend::updateGroupChat(PDU *pdu)
{
    if (pdu == NULL){
        return;
    }

    char caSendName[32] = {'\0'};
    strcpy(caSendName, pdu->caData);
    QString strMsg = QString("%1 says: %2").arg(caSendName).arg((char*)pdu->caMsg);
    m_pShowMsgTE->append(strMsg);
}

QListWidget *Friend::getFriendList()
{
    return m_pFriendListWidget;
}

void Friend::showOnline()
{
    if (m_pOnline->isHidden()){
        m_pOnline->show();

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else {
        m_pOnline->hide();
    }
}

void Friend::searchUsr()
{
    m_strSearchName = QInputDialog::getText(this, "搜索", "用户名:");
    if (!m_strSearchName.isEmpty()){
        qDebug() << m_strSearchName;

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_REQUEST;
        memcpy(pdu->caData, m_strSearchName.toStdString().c_str(), m_strSearchName.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Friend::flushFriend()
{
    QString strName = TcpClient::getInstance().LoginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    memcpy(pdu->caData, strName.toStdString().c_str(), strName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::delFriend()
{
    if (m_pFriendListWidget->currentItem() == NULL){
        QMessageBox::warning(this, "删除好友", "请选择要删除的好友");
        return;
    }

    QString strFriendName = m_pFriendListWidget->currentItem()->text();
    QString strSelfName = TcpClient::getInstance().LoginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
    memcpy(pdu->caData, strSelfName.toStdString().c_str(), strSelfName.size());
    memcpy(pdu->caData + 32, strFriendName.toStdString().c_str(), strFriendName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Friend::privateChat()
{
    if (m_pFriendListWidget->currentItem() != NULL){
        QString strChatName = m_pFriendListWidget->currentItem()->text();
        PrivateChat::getInstance().setChatName(strChatName);
        if (PrivateChat::getInstance().isHidden()){
            PrivateChat::getInstance().show();
        }
    }
    else {
        QMessageBox::warning(this, "警告", "请选择聊天对象");
    }
}

void Friend::groupChat()
{
    QString data = m_pInputMsgLE->text();
    m_pInputMsgLE->clear();

    QString strSendName = TcpClient::getInstance().LoginName();
    PDU *pdu = mkPDU(data.size() * 32);
    pdu->uiMsgType = ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
    memcpy(pdu->caMsg, data.toStdString().c_str(), data.size());
    memcpy(pdu->caData, strSendName.toStdString().c_str(), strSendName.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}
