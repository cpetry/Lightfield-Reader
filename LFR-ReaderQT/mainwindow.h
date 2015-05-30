#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "myqgraphicsview.h"
#include "qopengl_lfviewer.h"

#include "lfp_reader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void addTabMetaInfos( QString header, QString sha1, int sec_length,
                         QString metainfos, QString TabString);

    void addTabImage(QString header, QString sha1, int sec_length, QImage image,
                   bool raw_image, LFP_Reader::lf_meta meta_infos);

public slots:
    void chooseLFP();
    void chooseLFImage();

private:
    Ui::MainWindow *ui;
    MyGraphicsView* color_view, *microlens_view;
    QTabWidget* tabWidget;
    QOpenGL_LFViewer *opengl_viewer;
    LFP_Reader reader;
};

#endif // MAINWINDOW_H
