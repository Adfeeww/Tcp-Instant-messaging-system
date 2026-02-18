#include "tcpclient.h"
#include <QApplication>
#include "sharefile.h"
#include "opewidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    TcpClient w;
//    w.show();
    TcpClient::getInstance().show();

//    OpeWidget::getInstance().show();

//    ShareFile w;
//    w.show();
    return a.exec();
}
