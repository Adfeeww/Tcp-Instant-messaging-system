#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <opewidget.h>
#include <friend.h>
#include "sharefile.h"

Book::Book(QWidget *parent) : QWidget(parent)
{
    m_bDownload = false;

    m_pCurPath = TcpClient::getInstance().curPath();

    m_pTimer = new QTimer;

    m_pBookListW = new QListWidget;

    m_pReturnPB = new QPushButton("返回");
    m_pCreateDirPB = new QPushButton("创建文件夹");
    m_pDelDirPB = new QPushButton("删除文件夹");
    m_pRenamePB = new QPushButton("重命名文件");
    m_pFlushFilePB = new QPushButton("刷新文件夹");

    QVBoxLayout *m_leftLayout = new QVBoxLayout;
    m_leftLayout->addWidget(m_pReturnPB);
    m_leftLayout->addWidget(m_pCreateDirPB);
    m_leftLayout->addWidget(m_pDelDirPB);
    m_leftLayout->addWidget(m_pRenamePB);
    m_leftLayout->addWidget(m_pFlushFilePB);

    m_pUploadPB = new QPushButton("上传文件");
    m_pDownloadPB = new QPushButton("下载文件");
    m_pDelFilePB = new QPushButton("删除文件");
    m_pShareFilePB = new QPushButton("分享文件");

    QVBoxLayout *m_rightLayout = new QVBoxLayout;
    m_rightLayout->addWidget(m_pUploadPB);
    m_rightLayout->addWidget(m_pDownloadPB);
    m_rightLayout->addWidget(m_pDelFilePB);
    m_rightLayout->addWidget(m_pShareFilePB);

    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(m_leftLayout);
    pMain->addLayout(m_rightLayout);

    setLayout(pMain);

    connect(m_pCreateDirPB, SIGNAL(clicked(bool)), this, SLOT(createDir()));
    connect(m_pFlushFilePB, SIGNAL(clicked(bool)), this, SLOT(flushFile()));
    connect(m_pDelDirPB, SIGNAL(clicked(bool)), this, SLOT(deleteDir()));
    connect(m_pRenamePB, SIGNAL(clicked(bool)), this, SLOT(renameFile()));
    connect(m_pBookListW, &QListWidget::itemDoubleClicked, this, &Book::enterDir);
    connect(m_pReturnPB, SIGNAL(clicked(bool)), this, SLOT(returnDir()));
    connect(m_pUploadPB, SIGNAL(clicked(bool)), this, SLOT(uploadFile()));
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(uploadFileData()));
    connect(m_pDelFilePB, SIGNAL(clicked(bool)), this, SLOT(deleteFile()));
    connect(m_pDownloadPB, SIGNAL(clicked(bool)), this, SLOT(downLoadFile()));
    connect(m_pShareFilePB, SIGNAL(clicked(bool)), this, SLOT(shareFile()));
}

void Book::createDir()
{
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "新文件夹名");

    if (!strNewDir.isEmpty()){
        if(strNewDir.size() > 32){
            QMessageBox::warning(this, "警告", "文件夹名不能超过32个字符");
        }
        else {
            QString strCurPath = TcpClient::getInstance().curPath();

            PDU *pdu = mkPDU(0);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;

            memcpy(pdu->caData + 32, strNewDir.toStdString().c_str(), 32);
            memcpy(pdu->caData, strCurPath.toStdString().c_str(), 32);

            TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
    }
    else {
        QMessageBox::warning(this, "警告", "文件夹名不能为空");
    }
}

void Book::flushFile()
{
    m_pCurPath = TcpClient::getInstance().curPath();
    QString strCurPath = TcpClient::getInstance().curPath();
    QByteArray ba = strCurPath.toUtf8();

    PDU *pdu = mkPDU(ba.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    memcpy(pdu->caMsg, ba.constData(), ba.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::updateFileList(PDU *pdu)
{
    if (pdu == NULL){
        return;
    }

    m_pBookListW->clear();

    uint Size = pdu->uiMsgLen / sizeof(FileInfo);
    FileInfo *pFileInfo = (FileInfo*)pdu->caMsg;
    for (uint i = 0; i < Size; i ++){
        QString fileName = QString::fromUtf8(pFileInfo[i].caFileName);

        QListWidgetItem *pItem = new QListWidgetItem;
        if (pFileInfo[i].iFileType == 0){
            pItem->setIcon(QIcon(QPixmap(":/o1.png")));
        }
        else if (pFileInfo[i].iFileType == 1){
            pItem->setIcon(QIcon(QPixmap(":/note.png")));
        }

        pItem->setText(fileName);
        m_pBookListW->addItem(pItem);
    }
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}

void Book::setDownloadStatus(bool status)
{
    m_bDownload = status;
}

void Book::deleteDir()
{
    if (m_pBookListW->currentItem() == NULL){
        QMessageBox::warning(this, "警告", "请选择要删除的文件");
        return;
    }

    QString filePath = m_pBookListW->currentItem()->text();
    QByteArray ba = filePath.toUtf8();
    int len = ba.size();

    PDU *pdu = mkPDU(len + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_DIR_REQUEST;
    memcpy(pdu->caMsg, ba.constData(), len);

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
}

void Book::renameFile()
{
    if (m_pBookListW->currentItem() == NULL){
        QMessageBox::warning(this, "警告", "请选择重命名对象");
    }
    else {
        QString newName = QInputDialog::getText(this, "重命名", "请输入新名称");
        QString oldName = m_pBookListW->currentItem()->text();

        if (newName.isEmpty()){
            return;
        }

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_REQUEST;

        QByteArray oldBa = oldName.toUtf8();
        QByteArray newBa = newName.toUtf8();

        memset(pdu->caData, 0, 64);
        memcpy(pdu->caData, oldBa.constData(), qMin(31, oldBa.size()));
        memcpy(pdu->caData + 32, newBa.constData(), qMin(31, newBa.size()));

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);

        free(pdu);
    }
}

void Book::enterDir(QListWidgetItem *Item)
{
    if (Item == NULL){
        return;
    }

    QString dirName = Item->text();
    m_pCurPath += "/" + dirName;
    QByteArray ba = m_pCurPath.toUtf8();

    PDU *pdu = mkPDU(ba.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    memcpy(pdu->caMsg, ba.constData(), ba.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
}

void Book::returnDir()
{
    if (m_pCurPath.contains("/")){
        m_pCurPath = m_pCurPath.left(m_pCurPath.lastIndexOf("/"));

        QByteArray ba = m_pCurPath.toUtf8();
        PDU *pdu = mkPDU(ba.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
        memcpy(pdu->caMsg, ba.constData(), ba.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
}

void Book::uploadFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    m_strUploadFilePath = QFileDialog::getOpenFileName();

    QByteArray ba = strCurPath.toUtf8();

    if (!m_strUploadFilePath.isEmpty()){
        int index = m_strUploadFilePath.lastIndexOf('/');
        QString strFileName = m_strUploadFilePath.right(m_strUploadFilePath.size() - 1 - index);

        QFile file(m_strUploadFilePath);
        qint64 fileSize = file.size();

        PDU *pdu = mkPDU(ba.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        memcpy(pdu->caMsg, ba.constData(), ba.size());
        pdu->caMsg[ba.size()] = '\0';
        sprintf(pdu->caData, "%s %lld", strFileName.toUtf8().constData(), fileSize);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

        m_pTimer->start(1000);
    }
    else {
        QMessageBox::warning(this, "上传文件", "上传文件名不能为空");
    }
}

void Book::uploadFileData()
{
    m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    if (!file.open(QIODevice::ReadOnly)){
        QMessageBox::warning(this, "警告", "打开文件失败");
        return;
    }

    char *pBuffer = new char[4096];
    qint64 ret = 0;
    while(true){
        ret = file.read(pBuffer, 4096);
        if (ret > 0 && ret <= 4096){
            TcpClient::getInstance().getTcpSocket().write(pBuffer, ret);
        }
        else if (ret == 0){
            break;
        }
        else {
            QMessageBox::warning(this, "警告", "上传文件失败");
            break;
        }
    }
    file.close();
    delete []pBuffer;
    pBuffer = NULL;
}

void Book::deleteFile()
{
    if (m_pBookListW->currentItem() == NULL){
        QMessageBox::warning(this, "警告", "请选择要删除的文件");
    }

    QString delFileName = m_pBookListW->currentItem()->text();
    QString curPath = TcpClient::getInstance().curPath();

    QByteArray ba = delFileName.toUtf8();
    QByteArray curBA = curPath.toUtf8();

    PDU *pdu = mkPDU(curBA.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FILE_REQUEST;
    memcpy(pdu->caData, ba.constData(), ba.size());
    memcpy(pdu->caMsg, curBA.constData(), curBA.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}

void Book::downLoadFile()
{
    if (m_pBookListW->currentItem() == NULL){
        QMessageBox::warning(this, "警告", "请选择要下载的文件");
        return;
    }

    QString curPath = TcpClient::getInstance().curPath();
    QString downFileName = m_pBookListW->currentItem()->text();
    QString saveFilePath = QFileDialog::getSaveFileName();

    QByteArray curPathBA = curPath.toUtf8();
    QByteArray downFileNameBA = downFileName.toUtf8();

    if (!saveFilePath.isEmpty()){
        m_strSaveFilePath = saveFilePath;

        PDU *pdu = mkPDU(curPathBA.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;

        memcpy(pdu->caData, downFileNameBA.constData(), downFileNameBA.size());
        memcpy(pdu->caMsg, curPathBA.constData(), curPathBA.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else {
        QMessageBox::warning(this, "警告", "请选择下载文件的路径");
        m_strSaveFilePath.clear();
    }
}

void Book::shareFile()
{
    QListWidget *m_pFriendList = OpeWidget::getInstance().getFriend()->getFriendList();
    ShareFile::getInstance().updateFriend(m_pFriendList);
    if (ShareFile::getInstance().isHidden()){
        ShareFile::getInstance().show();
    }
}
