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
    static QImage EveryMat2QImageCol(const cv::Mat &src) {
            double scale = 255.0;
            QImage dest(src.cols, src.rows, QImage::Format_ARGB32);
            for (int y = 0; y < src.rows; ++y) {
                    QRgb *destrow = (QRgb*)dest.scanLine(y);
                    for (int x = 0; x < src.cols; ++x) {
                        // RGB
                        if (src.channels() >= 3){
                            if (src.type() == CV_8U) // CHAR
                                destrow[x] = qRgba(src.at<cv::Vec3b>(y,x)[2],
                                                src.at<cv::Vec3b>(y,x)[1],
                                                src.at<cv::Vec3b>(y,x)[0], 255);
                            else                    // FLOAT
                                destrow[x] = qRgba(src.at<cv::Vec3f>(y,x)[2] * scale,
                                                src.at<cv::Vec3f>(y,x)[1] * scale,
                                                src.at<cv::Vec3f>(y,x)[0] * scale, 255);
                        }
                        // GRAY
                        else{
                            if (src.type() == CV_8U){   //CHAR
                                unsigned int color = src.at<char>(y,x) * scale;
                                destrow[x] = qRgba(color, color, color, 255);
                            }
                            else{                       //FLOAT
                                unsigned int color = src.at<float>(y,x) * scale;
                                destrow[x] = qRgba(color, color, color, 255);
                            }
                        }
                    }
            }
            return dest;
    }

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

public slots:
    void loadImage();
    void updateLabel();


    void setViewMode(QString v){ this->view_mode = v.toStdString(); updateLabel();}
    void setSobelScale(double sc){ this->sobel_scale = sc; updateLabel();}
    void setSobelKernel(int sk){ this->sobel_k_size = sk; updateLabel();}
    void setGaussSigma(double gs){ this->gauss_sigma = gs; updateLabel();}
    void setGaussKernel(int gk){ this->gauss_k_size = gk; updateLabel();}


    static cv::Mat translateImg(cv::Mat &img, int offsetx, int offsety){
        cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, offsetx, 0, 1, offsety);
        cv::warpAffine(img,img,trans_mat,img.size());
        return trans_mat;
    }

protected:
    MyGraphicsView* view;
    cv::Mat input_img, output_img;
    const int size_d = 10;

private:
    virtual void calculateDepth() = 0;
    void stereoLikeTaxonomy();
    cv::Mat generateDepthMapFromDisparity(cv::Mat dis);
    void generateFromUVST(bool show_epi = false);
    std::pair<cv::Mat, cv::Mat> calculateDisparityFromEPI(cv::Mat epi, std::string result = "");

    std::string view_mode = "";

    int sobel_k_size = 3;
    float sobel_scale = 1.0f;
    int gauss_k_size = 9;
    float gauss_sigma = 2.0f;
};

#endif // IMAGEDEPTH_H
