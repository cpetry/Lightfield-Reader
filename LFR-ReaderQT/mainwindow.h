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
    void setupView(QImage image);
    void addTextTab(QWidget* widget, QString text);

private slots:
    void buttonDemosaicClicked();
    void buttonBayerClicked();
    void buttonSaveClicked();

private:
    Ui::MainWindow *ui;
    MyGraphicsView* view;
};

#endif // MAINWINDOW_H
