#include "opedb.h"
#include <QMessageBox>
#include <QDebug>
#include "protocol.h"

OpeDB::OpeDB(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setHostName("localhost");
    //数据库位置
    m_db.setDatabaseName("C:\\Users\\manong\\Desktop\\1\\sqlite3\\cloud.db");

    if (m_db.open()){
        QSqlQuery query;
        query.exec("SELECT * FROM usrInfo");
        while(query.next()){
            QString data = QString("%1,%2,%3,%4")
                    .arg(query.value(0).toString())
                    .arg(query.value(1).toString())
                    .arg(query.value(2).toString())
                    .arg(query.value(3).toString());

            qDebug() << data;
        }
    }
    else {
        QMessageBox::critical(NULL, "错误", "打开数据失败");
    }
}

OpeDB::~OpeDB()
{
    m_db.close();
}

bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if (name == NULL || pwd == NULL){
        return false;
    }

    QString data = QString("INSERT INTO usrInfo(name, pwd) values(\'%1\', \'%2\')").arg(name).arg(pwd);
    QSqlQuery query;
    return query.exec(data);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (name == NULL || pwd == NULL){
        return false;
    }

    QSqlQuery query;
    query.prepare("SELECT 1 FROM usrInfo where name = ? AND pwd = ? AND online = 0 LIMIT 1");
    query.addBindValue(name);
    query.addBindValue(pwd);

    if (!query.exec()){
        qDebug() << query.lastError().text();
        return false;
    }

    if (query.next()){
        QSqlQuery up;
        up.prepare("UPDATE usrInfo SET online = 1 where name = ? AND pwd = ?");
        up.addBindValue(name);
        up.addBindValue(pwd);

        if (!up.exec()) {
            qDebug() << "set online failed:" << up.lastError().text();
            return false;
        }

        return true;
    }

    return false;
}

void OpeDB::handleOffline(const char *name)
{
    if (name == NULL){
        return;
    }

    QSqlQuery query;
    query.prepare("UPDATE usrInfo SET online = 0 WHERE name = ?");
    query.addBindValue(name);
    query.exec();
}

QStringList OpeDB::handleOnline()
{
    QSqlQuery query;
    query.prepare("SELECT name FROM usrInfo WHERE online = 1");
    query.exec();

    QStringList result;
    while(query.next()){
        result.append(query.value(0).toString());
    }

    return result;
}

int OpeDB::handleSearchUsr(const char *name)
{
    if (name == NULL){
        return -1;
    }

    QSqlQuery query;
    query.prepare("SELECT online FROM usrInfo WHERE name = ?");
    query.addBindValue(name);
    query.exec();

    if (query.next()) {
        int ret = query.value(0).toInt();
        return ret;
    }
    else {
        return -1;
    }
}

int OpeDB::handleAddFriend(const char *pername, const char *name)
{
    if (pername == NULL || name == NULL) {
        return -1;
    }

    QSqlQuery query;
    query.prepare("SELECT * FROM friend WHERE (id = (SELECT id FROM usrInfo WHERE name = ?) AND friendId = (SELECT id FROM usrInfo WHERE name = ?)) OR (id = (SELECT id FROM usrInfo WHERE name = ?) AND friendId = (SELECT id FROM usrInfo WHERE name = ?))");
    query.addBindValue(pername);
    query.addBindValue(name);
    query.addBindValue(name);
    query.addBindValue(pername);
    query.exec();

    if (query.next()){
        return 0;   //已是好友
    }

    return 1;   //不是好友
}

void OpeDB::handleAgreeFriend(const char *name, const char *pername)
{
    if (name == NULL || pername == NULL){
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO friend (id, friendId) values ((SELECT id FROM usrInfo WHERE name = ?), (SELECT id FROM usrInfo WHERE name = ?))");
    query.addBindValue(name);
    query.addBindValue(pername);
    query.exec();
}

QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;

    if (!name) return strFriendList;

    QSqlQuery query;
    query.prepare(R"(
        SELECT u.name
        FROM usrInfo u
        WHERE /*u.online = 1
          AND*/ u.id IN (
              SELECT friendId FROM friend
              WHERE id = (SELECT id FROM usrInfo WHERE name = ?)
              UNION
              SELECT id FROM friend
              WHERE friendId = (SELECT id FROM usrInfo WHERE name = ?)
          )
    )");

    query.addBindValue(name);
    query.addBindValue(name);

    if (!query.exec()) {
        qDebug() << "SQL error:" << query.lastError();
        return strFriendList;
    }

    while (query.next()) {
        strFriendList << query.value(0).toString();
    }

    return strFriendList;
}

bool OpeDB::handleDeleteFriend(const char *name, const char *pername)
{
    if (name == NULL || pername == NULL){
        return false;
    }

    QSqlQuery query;
    query.prepare(R"(
                  DELETE FROM friend WHERE
                  (
                    id = (SELECT id FROM usrInfo WHERE name=?) AND
                    friendId = (SELECT id FROM usrInfo WHERE name=?)
                  )
                  OR
                  (
                    id = (SELECT id FROM usrInfo WHERE name=?) AND
                    friendId = (SELECT id FROM usrInfo WHERE name=?)
                  ))");
    query.addBindValue(name);
    query.addBindValue(pername);
    query.addBindValue(pername);
    query.addBindValue(name);
    query.exec();
    return true;
}


