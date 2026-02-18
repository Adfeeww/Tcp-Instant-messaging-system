#ifndef FRIEND_H
#define FRIEND_H

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include "protocol.h"

class Online;

class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = nullptr);

    void showAllOnlineUsr(PDU *pdu);
    void updateFriendList(PDU* pdu);
    void updateGroupChat(PDU* pdu);

    QString m_strSearchName;

    QListWidget *getFriendList();

signals:

public slots:
    void showOnline();
    void searchUsr();
    void flushFriend();
    void delFriend();
    void privateChat();
    void groupChat();

private:
    QTextEdit *m_pShowMsgTE;
    QListWidget *m_pFriendListWidget;
    QLineEdit *m_pInputMsgLE;

    QPushButton *m_pDelFriendPB;
    QPushButton *m_pFlushFriendPB;
    QPushButton *m_pShowOnlineUsrPB;
    QPushButton *m_pSearchPB;
    QPushButton *m_pMsgSendPB;
    QPushButton *m_pPrivateChatPB;

    Online *m_pOnline;
};

#endif // FRIEND_H
