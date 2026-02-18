#include "sharefile.h"

ShareFile::ShareFile(QWidget *parent) : QWidget(parent)
{
    m_pSelectAllPB = new QPushButton("全选");
    m_pCancelSelectPB = new QPushButton("取消选择");

    m_pOKPB = new QPushButton("确定");
    m_pCancelPB = new QPushButton("取消");

    m_pSA = new QScrollArea;
    m_FriendW = new QWidget;
    m_ButtonGroup = new QButtonGroup(m_FriendW);
    m_ButtonGroup->setExclusive(false);

    p = new QVBoxLayout(m_FriendW);

    QHBoxLayout *pTopLayout = new QHBoxLayout;
    pTopLayout->addWidget(m_pSelectAllPB);
    pTopLayout->addWidget(m_pCancelSelectPB);
    pTopLayout->addStretch();

    QHBoxLayout *pBottonLayout = new QHBoxLayout;
    pBottonLayout->addWidget(m_pOKPB);
    pBottonLayout->addWidget(m_pCancelPB);

    QVBoxLayout *pVB = new QVBoxLayout;
    pVB->addLayout(pTopLayout);
    pVB->addWidget(m_pSA);
    pVB->addLayout(pBottonLayout);

    setLayout(pVB);

    connect(m_pSelectAllPB, SIGNAL(clicked(bool)), this, SLOT(selectAll()));
    connect(m_pCancelSelectPB, SIGNAL(clicked(bool)), this, SLOT(cancelAll()));
}

ShareFile &ShareFile::getInstance()
{
    static ShareFile instance;
    return instance;
}

void ShareFile::updateFriend(QListWidget *m_pFriendList)
{
    if (m_pFriendList == NULL){
        return;
    }

    QAbstractButton *tmp = NULL;
    QList<QAbstractButton*> preButtons = m_ButtonGroup->buttons();
    for (int i = 0; i < preButtons.size(); i ++){
        p->removeWidget(tmp);
        m_ButtonGroup->removeButton(tmp);
        preButtons.removeOne(tmp);
        delete tmp;
        tmp = NULL;
    }

    QCheckBox *pCB = NULL;
    for (int i = 0; i < m_pFriendList->count(); i ++){
        pCB = new QCheckBox(m_pFriendList->item(i)->text());
        p->addWidget(pCB);
        m_ButtonGroup->addButton(pCB);
    }
    m_pSA->setWidget(m_FriendW);
}

void ShareFile::selectAll()
{
    QList<QAbstractButton*> btGroup = m_ButtonGroup->buttons();
    for (int i = 0; i < btGroup.size(); i ++){
        btGroup[i]->setChecked(true);
    }
}

void ShareFile::cancelAll()
{
    QList<QAbstractButton*> btGroup = m_ButtonGroup->buttons();
    for (int i = 0; i < btGroup.size(); i ++){
        btGroup[i]->setChecked(false);
    }
}
