#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();



    // при запуске приложения -> позволить пользователю выбрать файл

    return a.exec();
}
