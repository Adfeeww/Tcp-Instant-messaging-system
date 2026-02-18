#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class TcpServer; }
QT_END_NAMESPACE

class TcpServer : public QWidget
{
    Q_OBJECT

public:
    TcpServer(QWidget *parent = nullptr);
    ~TcpServer();

    //获取配置信息
    void loadConfig();

private:
    Ui::TcpServer *ui;

    QString m_strIP;
    quint16 m_usPort;
};
#endif // TCPSERVER_H
