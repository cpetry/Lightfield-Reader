#include "calibration.h"
#include "qopengl_lfviewer.h"
#include "mainwindow.h"
#include <QHBoxLayout>

cv::Mat calibration::createCalibrationImage(QStringList raw_images){
    cv::Mat input_img;
    cv::Mat averaged_img = cv::imread(raw_images[0].toStdString());
    averaged_img.convertTo(averaged_img, CV_32FC3);
    averaged_img = averaged_img.mul(1.0f/raw_images.length());

    for(int i=1; i <raw_images.length(); i++){
        input_img = cv::imread(raw_images[i].toStdString());
        input_img.convertTo(input_img, CV_32FC3);
        averaged_img += input_img.mul(1.0f/raw_images.length());
    }

    return averaged_img;
}

cv::Mat calibration::demosaicCalibrationImage(MainWindow *w, QString filename, LFP_Reader::lf_meta meta_info, int mode){
    QHBoxLayout* view_layout = new QHBoxLayout();
    QWidget* view_widget = new QWidget();
    view_widget->setLayout(view_layout);
    QOpenGL_LFViewer* viewer = new QOpenGL_LFViewer(w, filename, false, meta_info);
    view_layout->addWidget(viewer);
    w->tabWidget->addTab(view_widget,"View");
    viewer->buttonDemosaicClicked();
    viewer->saveImage();
    cv::Mat demosaiced_img;// = viewer->getDemosaicedImage(mode);
    return demosaiced_img;
}

float calibration::cho_EstimateRotation(QString processed_white_image){
    cv::Mat I = cv::imread(processed_white_image.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
    if( I.empty())
        return -1;

    cv::Mat padded;                            //expand input image to optimal size
    int m = cv::getOptimalDFTSize( I.rows );
    int n = cv::getOptimalDFTSize( I.cols ); // on the border add zero values
    cv::copyMakeBorder(I, padded, 0, m - I.rows, 0, n - I.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

    cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
    cv::Mat complexI;
    cv::merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

    cv::dft(complexI, complexI);            // this way the result may fit in the source matrix

    // compute the magnitude and switch to logarithmic scale
    // => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
    cv::split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
    cv::magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
    cv::Mat magI = planes[0];

    magI += cv::Scalar::all(1);                    // switch to logarithmic scale
    cv::log(magI, magI);

    // crop the spectrum, if it has an odd number of rows or columns
    magI = magI(cv::Rect(0, 0, magI.cols & -2, magI.rows & -2));

    // rearrange the quadrants of Fourier image so that the origin is at the image center
    int cx = magI.cols/2;
    int cy = magI.rows/2;

    cv::Mat q0(magI, cv::Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
    cv::Mat q1(magI, cv::Rect(cx, 0, cx, cy));  // Top-Right
    cv::Mat q2(magI, cv::Rect(0, cy, cx, cy));  // Bottom-Left
    cv::Mat q3(magI, cv::Rect(cx, cy, cx, cy)); // Bottom-Right

    cv::Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);

    q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
    q2.copyTo(q1);
    tmp.copyTo(q2);

    cv::normalize(magI, magI, 0, 1, CV_MINMAX); // Transform the matrix with float values into a
                                            // viewable image form (float between values 0 and 1).

    //cv::imshow("Input Image"       , I   );    // Show the result
    //cv::imshow("spectrum magnitude", magI);

    cv::normalize(magI, magI, 0, 255, CV_MINMAX );

    cv::imwrite("frequency_domain.png", magI);

    return 0;
}

