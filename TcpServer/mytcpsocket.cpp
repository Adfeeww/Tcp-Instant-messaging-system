#include "mytcpsocket.h"
#include "mytcpserver.h"
#include <QDebug>
#include <QDir>

MyTcpSocket::MyTcpSocket()
{
    m_bUpload = false;

    pTimer = new QTimer;

    connect(this, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    connect(this, SIGNAL(disconnected()), this, SLOT(clientOffline()));
    connect(pTimer, SIGNAL(timeout()), this, SLOT(sendFileMsg()));
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::recvMsg()
{
    if (!m_bUpload){
        qDebug() << this->bytesAvailable();
        uint uiPDULen = 0;
        this->read((char*)&uiPDULen, sizeof(uint));
        uint uiMsgLen = uiPDULen - sizeof (PDU);
        PDU* pdu = mkPDU(uiMsgLen);
        this->read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint));
        switch (pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
            if (ret){
                strcpy(respdu->caData, REGIST_OK);
                QDir dir;
                dir.mkdir(QString("./%1").arg(caName));
            }
            else {
                strcpy(respdu->caData, REGIST_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
            if (ret){
                strcpy(respdu->caData, LOGIN_OK);
                m_strName = caName;
            }
            else {
                strcpy(respdu->caData, LOGIN_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
        {
            QStringList ret = OpeDB::getInstance().handleOnline();
            uint uiMsgLen = ret.size() * 32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for (int i = 0; i < ret.size(); i ++){
                memcpy((char*)respdu->caMsg + i * 32, ret.at(i).toStdString().c_str(), ret.at(i).size());
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
        {
            int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if (ret == -1){
                strcpy(respdu->caData, SEARCH_USR_NO);
            }
            else if (ret == 0){
                strcpy(respdu->caData, SEARCH_USR_OFFLINE);
            }
            else if (ret == 1){
                strcpy(respdu->caData, SEARCH_USR_ONLINE);
            }
            this->write((char*)respdu, pdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32);
            strncpy(caName, pdu->caData + 32, 32);
            int ret = OpeDB::getInstance().handleAddFriend(caPerName, caName);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            if (ret == -1){
                strcpy(respdu->caData, UNKONW_ERROR);
            }
            else if (ret == 0){     //已是好友
                strcpy(respdu->caData, EXISTED_FRIEND);
            }
            else if (ret == 1){     //不是好友
                MyTcpServer::getInstance().resend(caPerName, pdu);
            }

            if (ret == -1 || ret == 0){
                write((char*)respdu, respdu->uiPDULen);
            }

            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            OpeDB::getInstance().handleAgreeFriend(caName, m_strName.toStdString().c_str());
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList ret = OpeDB::getInstance().handleFlushFriend(caName);
            uint uiMsgLen = ret.size() * 32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;

            for (int i = 0; i < ret.size(); i ++){
                memcpy((char*)respdu->caMsg + i * 32, ret.at(i).toStdString().c_str(), ret.at(i).size());
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPerName, pdu->caData + 32, 32);
            OpeDB::getInstance().handleDeleteFriend(caName, caPerName);

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData, DELETE_FRIEND_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            MyTcpServer::getInstance().resend(caPerName, pdu);

            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            char strPerName[32] = {'\0'};
            memcpy(strPerName, pdu->caData + 32, 32);
            MyTcpServer::getInstance().resend(strPerName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            char caSendName[32] = {'\0'};
            strcpy(caSendName, pdu->caData);

            QStringList ret = OpeDB::getInstance().handleFlushFriend(caSendName);
            MyTcpServer::getInstance().sendGroup(ret, pdu);

            write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
        {
            QString strCurPath = QString("%1").arg(pdu->caData);
            qDebug() << strCurPath;
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;

            QDir dir;
            bool ret = dir.exists(strCurPath);
            if (ret){
                char caNewDir[32] = {'\0'};
                memcpy(caNewDir, pdu->caData + 32, 32);
                QString strNewPath = strCurPath + "/" + caNewDir;
                ret = dir.exists(strNewPath);
                if (ret){
                    strcpy(respdu->caData, FILE_NAME_EXIST);
                }
                else {
                    dir.mkdir(strNewPath);
                    strcpy(respdu->caData, CREATE_DIR_OK);
                }
            }
            else {
                strcpy(respdu->caData, DIR_NO_EXIST);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        {
            QString strPath = QString::fromUtf8(pdu->caMsg);
            QDir dir(strPath);

            QFileInfoList fileInfoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
            int iFileCount = fileInfoList.size();

            PDU *respdu = mkPDU(sizeof(FileInfo) * iFileCount);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;

            FileInfo* pFileInfo = (FileInfo*)respdu->caMsg;

            for (int i = 0; i < iFileCount; i ++){
                memcpy(pFileInfo[i].caFileName, fileInfoList[i].fileName().toUtf8(), 31);

                if (fileInfoList[i].isDir()){
                    pFileInfo[i].iFileType = 0;
                }
                else if (fileInfoList[i].isFile()){
                    pFileInfo[i].iFileType = 1;
                }
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_DELETE_DIR_REQUEST:
        {
            QByteArray ba(pdu->caMsg, pdu->uiMsgLen);
            QString fileName = QString::fromUtf8(ba);

            QString DelPath = QString("./%1/%2").arg(m_strName).arg(fileName);

            QDir dir(DelPath);
            bool ret = dir.removeRecursively();
            PDU *respdu = mkPDU(0);

            if (ret){
                respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_DIR_OK;
            }
            else {
                respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_DIR_FAILED;
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
        {
            QString strCurPath = QString("./%1").arg(m_strName);
            QString oldName = QString::fromUtf8(pdu->caData, 32).trimmed();
            QString newName = QString::fromUtf8(pdu->caData + 32, 32).trimmed();

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;

            QDir dir(strCurPath);
            bool ret = dir.rename(oldName, newName);
            if (ret){
                strcpy(respdu->caData, "rename success");
            }
            else {
                strcpy(respdu->caData, "fail to rename");
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
            char* pPath = new char[pdu->uiMsgLen];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            delete []pPath;
            pPath = NULL;

            m_file.setFileName(strPath);
            if (m_file.open(QIODevice::WriteOnly)){
                m_bUpload = true;
                m_iTotal = fileSize;
                m_iRecved = 0;
            }
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FILE_REQUEST:
        {
            char delFileName[32] = {'\0'};
            memcpy(delFileName, pdu->caData, 32);
            char *curPath = new char[pdu->uiMsgLen];
            memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);

            QString strPath = QString("%1/%2").arg(curPath).arg(delFileName);
            qDebug() << strPath;

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FILE_RESPOND;

            int ret = QFile::remove(strPath);
            if (ret){
                memcpy(respdu->caData, "删除成功", 32);
            }
            else {
                memcpy(respdu->caData, "删除失败", 32);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        {
            char *curPath = new char[pdu->uiMsgLen];
            char fileName[32] = {'\0'};

            memcpy(curPath, pdu->caMsg, pdu->uiMsgLen);
            memcpy(fileName, pdu->caData, 32);

            QString downPath = QString("%1/%2").arg(curPath).arg(fileName);

            QFileInfo fileInfo(downPath);
            qint64 fileSize = fileInfo.size();

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            snprintf(respdu->caData, sizeof(respdu->caData), "%s %lld", fileName, fileSize);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            m_file.setFileName(downPath);
            m_file.open(QIODevice::ReadOnly);
            pTimer->start(1000);
            break;
        }
        default:
            break;
        }
        free(pdu);
        pdu = NULL;
    }
    else {
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        if (m_iTotal == m_iRecved){
            strcpy(respdu->caData, "upload file ok");

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (m_iTotal < m_iRecved){
            strcpy(respdu->caData, "upload file failured");

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        m_file.close();
        m_bUpload = false;
    }
}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}

void MyTcpSocket::sendFileMsg()
{
    pTimer->stop();

    char *pData = new char[4090];
    qint64 ret = 0;

    while(true){
        ret = m_file.read(pData, 4090);
        if (ret > 0 && ret <= 4090){
            write(pData, ret);
        }
        else if (ret == 0){
            m_file.close();
        }
        else {
            qDebug() << "发送文件内容给客户端的过程中失败";
            m_file.close();
            break;
        }
    }

    delete []pData;
    pData = NULL;
}
