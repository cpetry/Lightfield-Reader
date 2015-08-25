#include "depthredefinedisparity.h"

void DepthRedefineDisparity::loadImage(){
    QString file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Lightfield Image"),           // window name
                               "../",                                       // relative folder
                               QString("LightField Images(*.PNG *.JPG)"));  // filetype

    if (file.isEmpty()) // Do nothing if filename is empty
        return;

    input_img = cv::imread(file.toStdString());
    output_img = input_img;
    updateLabel();
}

void DepthRedefineDisparity::fillInHolesInDepth(){
    cv::Mat result = input_img.clone();

    if(option_small_holes)
        cv::medianBlur(result, result, option_median_kernel_size);

    if(option_middle_holes){

        /*cv::Mat channel[3];
        cv::split(result, channel);
        cv::Mat red = channel[2];*/
        cv::Mat imgHSV;
        cv::cvtColor(result, imgHSV, cv::COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

        // extract red mask
        cv::Mat lower_red_hue_range, upper_red_hue_range, imgThresholded;
        cv::inRange(imgHSV, cv::Scalar(0, 10, 10), cv::Scalar(25, 255, 255), lower_red_hue_range); //Threshold the image
        cv::inRange(imgHSV, cv::Scalar(150, 10, 10), cv::Scalar(179, 255, 255), upper_red_hue_range);
        cv::addWeighted(lower_red_hue_range, 1.0, upper_red_hue_range, 1.0, 0.0, imgThresholded);

        // invert mask to delete all red areas
        imgThresholded = 255 - imgThresholded;
        cv::Mat gray;
        cv::cvtColor(imgThresholded,gray,CV_GRAY2BGR);
        cv::bitwise_and(result, gray, result);

        // close all holes created by mask
        cv::Mat const structure_elem = cv::getStructuringElement(
                             cv::MORPH_ELLIPSE, cv::Size(option_close_kernel_size, option_close_kernel_size));
        cv::morphologyEx(result, result, cv::MORPH_CLOSE, structure_elem);
        cv::Mat black_areas;
        cv::inRange(result, cv::Scalar(0,0,0), cv::Scalar(0,0,0), black_areas);
        //result = imgThresholded;
        result.setTo(cv::Scalar(0,0,255), black_areas); // set holes to red again!
    }

    if (option_big_holes){
        int cn = result.channels();
        cv::Vec3b bgrPixel;
        cv::Vec3b bgrPixel_before;
        cv::Vec3b bgrPixel_after;

        for(int i = 0; i < result.cols; i++)
        {
            for(int j = 0; j < result.rows; j++)
            {
                bgrPixel = result.at<cv::Vec3b>(j, i);

                // completely red
                if (bgrPixel.val[0] == 0 && bgrPixel.val[1] == 0 && bgrPixel.val[2] > 240){
                    bgrPixel_before = (i>0) ? result.at<cv::Vec3b>(j, i-1) : cv::Vec3b(0,0,0);

                    bool pixel_after = false;
                    for(int ix = i; ix < i + option_big_holes_max_size && ix < result.cols; ix++){
                        if (result.at<cv::Vec3b>(j, ix).val[0] != 0
                        && result.at<cv::Vec3b>(j, ix).val[1] != 0
                        && result.at<cv::Vec3b>(j, ix).val[2] < 240){
                            bgrPixel_after = result.at<cv::Vec3b>(j, ix);
                            pixel_after = true;
                            ix = i + option_big_holes_max_size;
                        }
                    }

                    cv::Vec3b new_value(0,0,0);

                    // both values exist
                    if (pixel_after){
                        new_value = (bgrPixel_before.val[0] < bgrPixel_after.val[0]) ? bgrPixel_before : bgrPixel_after;
                        result.at<cv::Vec3b>(j, i) = new_value;
                    }
                    // before value exists
                    /*else if (j>0){
                        new_value = bgrPixel_before;
                        result.at<cv::Vec3b>(j, i) = new_value;
                    }*/



                }

                // do something with BGR values...
            }
        }
    }


    output_img = result;
    updateLabel();
}
