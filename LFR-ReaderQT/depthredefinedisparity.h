#ifndef DEPTHREDEFINEDISPARITY_H
#define DEPTHREDEFINEDISPARITY_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QLabel>
#include <QObject>
#include <QFileDialog>

#include "imagedepth.h"
#include "myqgraphicsview.h"

class DepthRedefineDisparity : public QObject
{
    Q_OBJECT
public:
    DepthRedefineDisparity(MyGraphicsView* view){
        this->view = view;
    }
    ~DepthRedefineDisparity(){}

public slots:
    void setOptionSmallHoles(bool b){option_small_holes = b; }
    void setOptionMiddleHoles(bool b){option_middle_holes = b; }
    void setOptionBigHoles(bool b){option_big_holes = b; }
    void setCloseKernel(int k){option_close_kernel_size = k;}
    void setMedianKernel(int k){option_median_kernel_size = k;}
    void setBigHolesMaxSize(int k){option_big_holes_max_size = k;}

    void loadImage();

    void updateLabel(){
        cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
        QImage output = ImageDepth::EveryMat2QImageCol(output_img);
        this->view->setPixmap(QPixmap::fromImage(output));
    }

    void fillInHolesInDepth();

private:

    MyGraphicsView* view;
    cv::Mat input_img, output_img;

    bool option_small_holes = true;
    bool option_middle_holes = true;
    bool option_big_holes = true;
    int option_close_kernel_size = 10;
    int option_median_kernel_size = 3;
    int option_big_holes_max_size = 50;
};

#endif // DEPTHREDEFINEDISPARITY_H
