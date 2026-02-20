#include "opedb.h"
#include <QMessageBox>
#include <QDebug>
#include <QSettings>

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

    loadConfig();

    if (m_db.open()){
        //启用外键支持(SQLite默认关闭外键)
        QSqlQuery query;
        query.exec("PRAGMA foreign_key = ON;");

        if (!createTables()){
            QMessageBox::warning(NULL, "错误", "创建数据库表失败");
        }

        QSqlQuery selectQuery;
        selectQuery.exec("SELECT * FROM usrInfo");
        while(selectQuery.next()){
            QString data = QString("%1,%2,%3,%4")
                    .arg(selectQuery.value(0).toString())
                    .arg(selectQuery.value(1).toString())
                    .arg(selectQuery.value(2).toString())
                    .arg(selectQuery.value(3).toString());

            qDebug() << data;
        }
    }
    else {
        QMessageBox::critical(NULL, "错误", "打开数据库失败");
    }
}

void OpeDB::loadConfig()
{
    QSettings settings("server.ini", QSettings::IniFormat);

    if (!settings.contains("database/path")){
        settings.setValue("database/path", "./cloud.db");
        settings.sync();
    }

    QString dbPath = settings.value("database/path").toString();
    m_db.setDatabaseName(dbPath);
}

bool OpeDB::createTables()
{
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS usrInfo("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT NOT NULL UNIQUE,"
                "pwd TEXT NOT NULL,"
                "online INTEGER DEFAULT 0)");

    if (query.lastError().isValid()){
        qDebug() << query.lastError().text();
    }

    query.exec("CREATE TABLE IF NOT EXISTS friend("
               "id INTEGER NOT NULL,"
               "friendId INTEGER NOT NULL,"
               "PRIMARY KEY(id, friendId),"
               "FOREIGN KEY (id) REFERENCES usrInfo (id) ON DELETE CASCADE,"
               "FOREIGN KEY (friendId) REFERENCES usrInfo (id) ON DELETE CASCADE)");

    if (query.lastError().isValid()){
        qDebug() << query.lastError().text();
    }

    //加索引，提高效率
    query.exec("CREATE INDEX IF NOT EXISTS idx_usrInfo_name ON usrInfo(name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_usrInfo_online ON usrInfo(online)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_friend_id ON friend(id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_friend_friendId ON friend(friendId)");

    return true;
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


