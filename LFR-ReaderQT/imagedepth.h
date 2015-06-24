#ifndef IMAGEDEPTH_H
#define IMAGEDEPTH_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QLabel>
#include <QObject>

#include "myqgraphicsview.h"

class ImageDepth : public QObject
{
    Q_OBJECT

public:
    ImageDepth(MyGraphicsView* view){
        this->view = view;
    }
    ~ImageDepth();


public slots:
    void updateLabel(QString);
    void loadImage();

private:
    void generateFromUVST();
    std::pair<cv::Mat, cv::Mat> calculateDisparityFromEPI(cv::Mat epi, std::string result = "");

    static QImage Mat2QImageCol(const cv::Mat3b &src) {
            QImage dest(src.cols, src.rows, QImage::Format_ARGB32);
            for (int y = 0; y < src.rows; ++y) {
                    const cv::Vec3b *srcrow = src[y];
                    QRgb *destrow = (QRgb*)dest.scanLine(y);
                    for (int x = 0; x < src.cols; ++x) {
                            destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
                    }
            }
            return dest;
    }

    static QImage Mat2QImage(const cv::Mat_<double> &src)
    {
            double scale = 255.0;
            QImage dest(src.cols, src.rows, QImage::Format_ARGB32);
            for (int y = 0; y < src.rows; ++y) {
                    const double *srcrow = src[y];
                    QRgb *destrow = (QRgb*)dest.scanLine(y);
                    for (int x = 0; x < src.cols; ++x) {
                            unsigned int color = srcrow[x] * scale;
                            destrow[x] = qRgba(color, color, color, 255);
                    }
            }
            return dest;
    }

    MyGraphicsView* view;
    cv::Mat input_img, output_img;
};

#endif // IMAGEDEPTH_H
