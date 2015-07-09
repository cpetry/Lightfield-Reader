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
    void updateLabel();
    void loadImage();
    void setViewMode(QString v){ this->view_mode = v.toStdString(); updateLabel();}
    void setSobelScale(double sc){ this->sobel_scale = sc; updateLabel();}
    void setSobelKernel(int sk){ this->sobel_k_size = sk; updateLabel();}
    void setGaussSigma(double gs){ this->gauss_sigma = gs; updateLabel();}
    void setGaussKernel(int gk){ this->gauss_k_size = gk; updateLabel();}

private:
    void stereoLikeTaxonomy();
    void costAwareDepthMapEstimation();
    cv::Mat generateDepthMapFromDisparity(cv::Mat dis);
    void generateFromUVST(bool show_epi = false);
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
    std::string view_mode = "";
    int sobel_k_size = 3;
    float sobel_scale = 1.0f;
    int gauss_k_size = 9;
    float gauss_sigma = 2.0f;
};

#endif // IMAGEDEPTH_H
