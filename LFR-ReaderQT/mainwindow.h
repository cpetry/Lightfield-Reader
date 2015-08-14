#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "lfp_raw_view.h"
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
    QTabWidget* tabWidget;

public slots:
    void chooseLFP();
    void chooseLFImage();
    void chooseConvertImages();
    void chooseExtractRawLFFolder();
    void chooseCreateCalibrationImage();
    void chooseCreateVideoFromPNGs();
    void chooseVideoPlayer();
    void chooseVideoFromImageSequence();
    void chooseCreate3DScene();
    void chooseGenerate_DepthMap();

    void chooseCho_EstimateCenterPositions();

private:
    Ui::MainWindow *ui;
    lfp_raw_view* color_view, *microlens_view;
    LFP_Reader reader;
};

#endif // MAINWINDOW_H
