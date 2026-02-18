#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include <QDir>
#include <QFile>
#include <QTimer>
#include "protocol.h"
#include "opedb.h"

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString getName();

signals:
    void offline(MyTcpSocket *mysocket);

public slots:
    void recvMsg();
    void clientOffline();
    void sendFileMsg();

private:
    QString m_strName;

    QFile m_file;
    qint64 m_iTotal;
    qint64 m_iRecved;
    bool m_bUpload;

    QTimer *pTimer;
};

#endif // MYTCPSOCKET_H
