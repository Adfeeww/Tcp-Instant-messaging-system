#ifndef PASSWORDUTILS_H
#define PASSWORDUTILS_H

#include <QString>

class PasswordUtils
{
public:
    PasswordUtils();

    static PasswordUtils &getInstance();

    QString hashPassword(const QString &pwd);
    bool verifyPassword(const QString &pwd, const QString &hashedPwd);
};

#endif // PASSWORDUTILS_H
