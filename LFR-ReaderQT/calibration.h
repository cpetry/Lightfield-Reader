#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QStringList>

#include "lfp_reader.h"
#include "imagedepth.h"

class calibration
{
public:

    static cv::Mat createCalibrationImage(QStringList raw_images);
    static cv::Mat demosaicCalibrationImage(MainWindow *w, QString filename, LFP_Reader::lf_meta meta_info, int mode);
    static cv::Mat spreadDemosaicedCalibrationImage(QString image);

    static float cho_EstimateRotation(QString processed_white_image);
    static cv::Mat cho_EstimateCenterPositions(QString processed_white_image);
};

#endif // CALIBRATION_H
