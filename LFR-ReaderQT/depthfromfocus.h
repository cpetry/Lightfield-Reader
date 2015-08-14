#ifndef DEPTHFROMFOCUS_H
#define DEPTHFROMFOCUS_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QLabel>
#include <QObject>

#include "myqgraphicsview.h"
#include "imagedepth.h"

class DepthFromFocus : public ImageDepth
{
    Q_OBJECT

public:
    DepthFromFocus(MyGraphicsView* view){
        this->view = view;
    }
    ~DepthFromFocus(){}

    void calculateDepth();
    cv::Mat createFocusedImage(const cv::Mat image, const int size_u, const int size_v, const float shift);
    cv::Mat createFocusVolume(const int size_u, const int size_v, const float threshold);

public slots:
    void setUseMaxFocus(bool b) { this->use_max_focus = b;  updateLabel();}
    void setUseMaxVariance(bool b) { this->use_max_variance = b;  updateLabel();}
    void setFocusThreshold(double v) { this->focus_threshold = v; updateLabel();}
    void setMaxVariance(double mv){ this->max_variance = mv; updateLabel();}


protected:
    float max_variance = 10.0f;
    double focus_threshold = 16.0f;
    cv::Mat focusCost;

    bool use_max_focus = false, use_max_variance = false;
};

#endif // DEPTHFROMFOCUS_H
