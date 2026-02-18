#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include "protocol.h"

class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);

    void updateFileList(PDU *pdu);
    QString getSaveFilePath();
    void setDownloadStatus(bool status);
    bool getDownloadStatus();

    qint64 m_iTotal;
    qint64 m_iRecved;

signals:

public slots:
    void createDir();
    void flushFile();
    void deleteDir();
    void renameFile();
    void enterDir(QListWidgetItem* Item);
    void returnDir();
    void uploadFile();
    void uploadFileData();
    void deleteFile();
    void downLoadFile();
    void shareFile();

private:
    QListWidget *m_pBookListW;
    QPushButton *m_pReturnPB;
    QPushButton *m_pCreateDirPB;
    QPushButton *m_pDelDirPB;
    QPushButton *m_pRenamePB;
    QPushButton *m_pFlushFilePB;
    QPushButton *m_pUploadPB;
    QPushButton *m_pDownloadPB;
    QPushButton *m_pDelFilePB;
    QPushButton *m_pShareFilePB;

    QString m_pCurPath;
    QString m_strUploadFilePath;
    QString m_strSaveFilePath;

    QTimer* m_pTimer;
    bool m_bDownload;
};

#endif // BOOK_H
