#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // when the application is started -> let the user choose a file
    w.chooseFile();

    return a.exec();
}
