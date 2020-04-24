#include "mainwindow.h"

#include <QApplication>
#include<windows.h>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Task Manger");
    w.show();
    return a.exec();
}
