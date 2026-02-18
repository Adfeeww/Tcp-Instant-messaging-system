#include "mytcpserver.h"
#include <QDebug>

MyTcpServer::MyTcpServer()
{

}

MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "new client connected";
    //动态创建一个对象，生命周期由我手动控制，知道我手动删除才结束，不受作用域影响
    MyTcpSocket *pTcpSocket = new MyTcpSocket;
    //将操作系统分配的套接字描述符绑定到该对象上，使该对象能够进行通信
    /*socketDescriptor:由操作系统已建立的连接分配的文件描述符，
    表示已连接的套接字*/
    pTcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(pTcpSocket);

    connect(pTcpSocket, SIGNAL(offline(MyTcpSocket*)), this, SLOT(deleteSocket(MyTcpSocket*)));
}

void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if (pername == NULL || pdu == NULL){
        return;
    }

    for (int i = 0; i < m_tcpSocketList.size(); i ++){
        if (pername == m_tcpSocketList.at(i)->getName()){
            m_tcpSocketList.at(i)->write((char*)pdu, pdu->uiPDULen);
            break;
        }
    }
}

void MyTcpServer::sendGroup(QStringList m_AllFriend, PDU *pdu)
{
    for (int i = 0; i < m_AllFriend.size(); i ++){
        for (int j = 0; j < m_tcpSocketList.size(); j ++){
            if (m_AllFriend.at(i) == m_tcpSocketList.at(j)->getName()){
                m_tcpSocketList.at(j)->write((char*)pdu, pdu->uiPDULen);
                break;
            }
        }
    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    QList<MyTcpSocket*>::iterator iter = m_tcpSocketList.begin();
    for (; iter != m_tcpSocketList.end(); iter ++){
        if (*iter == mysocket){
            (*iter)->deleteLater();
            *iter = NULL;
            m_tcpSocketList.erase(iter);
            break;
        }
    }
    for (int i = 0; i < m_tcpSocketList.size(); i ++){
        qDebug() << m_tcpSocketList.at(i)->getName();
    }
}
