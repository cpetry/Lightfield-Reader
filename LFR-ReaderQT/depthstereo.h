#ifndef DEPTHSTEREO_H
#define DEPTHSTEREO_H

#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/contrib/contrib.hpp"


#include <QImage>
#include <QLabel>
#include <QObject>
#include <QFileDialog>

#include "myqgraphicsview.h"
#include "imagedepth.h"

class DepthStereo : public ImageDepth
{
public:
    DepthStereo(MyGraphicsView* view){
        this->view = view;
    }
    ~DepthStereo(){}

public slots:

    void calculateDepth();

};

#endif // DEPTHSTEREO_H
