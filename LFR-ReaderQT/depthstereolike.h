#ifndef DEPTHSTEREOLIKE_H
#define DEPTHSTEREOLIKE_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QLabel>
#include <QObject>

#include "myqgraphicsview.h"
#include "imagedepth.h"

class DepthStereoLike: public ImageDepth
{
    Q_OBJECT

public:
    DepthStereoLike(MyGraphicsView* view){
        this->view = view;
    }
    ~DepthStereoLike(){}

    void calculateDepth();
    void old_disparity();

public slots:
    void setSobelScale(double sc){ this->sobel_scale = sc; updateLabel();}
    void setSobelKernel(int sk){ this->sobel_k_size = sk; updateLabel();}
    void setGaussSigma(double gs){ this->gauss_sigma = gs; updateLabel();}
    void setGaussKernel(int gk){ this->gauss_k_size = gk; updateLabel();}

private:
    void stereoLikeTaxonomy();
    cv::Mat calculateCostVolume(bool horizontal);
    std::pair<cv::Mat, cv::Mat> calculateDisparityFromEPI(cv::Mat epi, std::string result = "");

    int sobel_k_size = 3;
    float sobel_scale = 1.0f;
    int gauss_k_size = 9;
    float gauss_sigma = 2.0f;

    cv::Mat cost_vol_hori, cost_vol_vert;

};

#endif // DEPTHSTEREOLIKE_H
