#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "myqgraphicsview.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setupMetaInfos( QString header, QString sha1, int sec_length, QString metainfos, QString TabString);
    void setupView(QString header, QString sha1, int sec_length, QImage image, bool raw_image);
    void addTextTab(QWidget* widget, QString text);

public slots:
    void chooseFile();

private slots:
    void buttonGrayClicked();
    void buttonBayerClicked();
    void buttonDemosaicClicked();
    void buttonSaveClicked();


private:
    Ui::MainWindow *ui;
    MyGraphicsView* view;
};

#endif // MAINWINDOW_H
