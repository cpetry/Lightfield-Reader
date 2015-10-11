#include "imagedepth.h"

#include <QFileDialog>

void ImageDepth::loadImage(){

    QString file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Lightfield Image"),           // window name
                               "../",                                       // relative folder
                               QString("LightField Images(*.PNG *.JPG)"));  // filetype

    if (file.isEmpty()) // Do nothing if filename is empty
        return;

    // try to read meta info
    QString filename = file.split('.')[0];
    std::string txt_file = filename.toStdString() + ".txt";

    if (QFile(QString::fromStdString(txt_file)).exists()){
        LFP_Reader reader;
        std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
        std::string text = reader.readText(input);
        QString meta_text = QString::fromStdString(text);
        reader.parseLFMetaInfo(meta_text);
        this->meta_info = reader.meta_infos;
        std::cout << "Read in meta info file!\n" << txt_file << "\n";
    }
    else{
        std::cout << "Could not find meta info file!\n" << txt_file << "\n";
    }

    input_img = cv::imread(file.toStdString());
    output_img = input_img;
    setViewMode("input");
}

void ImageDepth::saveImage(){
    //glViewport((width - side) / 2, (height - side) / 2, side, side);
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");
    if (filename != ""){
        cv::Mat savemat( output_img.rows, output_img.cols, CV_8U );
        output_img.convertTo(savemat, CV_8U, 256.0);
        cv::imwrite(filename.toStdString(), savemat);
    }
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


