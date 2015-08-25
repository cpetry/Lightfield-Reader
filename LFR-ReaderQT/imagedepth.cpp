#include "imagedepth.h"

#include <QFileDialog>

void ImageDepth::loadImage(){
    QString file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Lightfield Image"),           // window name
                               "../",                                       // relative folder
                               QString("LightField Images(*.PNG *.JPG)"));  // filetype

    if (file.isEmpty()) // Do nothing if filename is empty
        return;

    input_img = cv::imread(file.toStdString());
    output_img = input_img;
    setViewMode("input");
}

void ImageDepth::updateLabel(){
    QImage output;
    calculateDepth();
    cv::normalize(output_img, output_img, 0, 1, CV_MINMAX, CV_32F);
    output = EveryMat2QImageCol(output_img);
    this->view->setPixmap(QPixmap::fromImage(output));
}





cv::Mat ImageDepth::generateDepthMapFromDisparity(cv::Mat dis){
    double f = 0.0115421 / (1.399*pow(10,-6)); // focal length (in pixel)
    float B = 0.00002f; // baseline (distance of two lenses, in meter)
    cv::Mat Z = dis.clone();
    for (int t=0; t < dis.rows; t++)
        for (int s=0; s < dis.cols; s++){
            float d = dis.at<float>(t,s);
            //Z.at<float>(t,s) = f*B/d;
            Z.at<float>(t,s) = d;
            //Z.at<float>(t,s) = dis.at<float>(t,s);
        }
    return Z;
}


