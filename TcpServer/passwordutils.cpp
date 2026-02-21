#include "passwordutils.h"
#include <QCryptographicHash>

PasswordUtils::PasswordUtils() {}

PasswordUtils &PasswordUtils::getInstance()
{
    static PasswordUtils instance;
    return instance;
}

QString PasswordUtils::hashPassword(const QString &pwd)
{
    //转为字节数组
    QByteArray hashpassword = pwd.toUtf8();
    QByteArray hash = QCryptographicHash::hash(hashpassword, QCryptographicHash::Sha256);

    return hash.toHex();
}

bool PasswordUtils::verifyPassword(const QString &pwd, const QString &hashedPwd)
{
    return hashPassword(pwd) == hashedPwd;
}

