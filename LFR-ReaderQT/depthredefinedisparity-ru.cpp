#include "depthredefinedisparity.h"

void DepthRedefineDisparity::loadImage(){
    QString file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Lightfield Image"),           // Имя окна
                               "../",                                       // относительная папка
                               QString("LightField Images(*.PNG *.JPG)"));  // тип файла

    if (file.isEmpty()) // Ничего не делать, если имя файла пусто
        return;

    lightfield_img = cv::imread(file.toStdString(), CV_LOAD_IMAGE_COLOR);


    file = QFileDialog::getOpenFileName(NULL ,
                               QString("Choose Depth map Image"),           // Имя окна
                               "../",                                       // относительная папка
                               QString("LightField Images(*.PNG *.JPG)"));  // тип файла

    if (file.isEmpty()) // Ничего не делать, если имя файла пусто
        return;

    input_img = cv::imread(file.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);

    output_img = input_img;
    updateLabel();
}

void DepthRedefineDisparity::saveImage(){
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

        // извлечь красную маску
        cv::Mat lower_red_hue_range, upper_red_hue_range, imgThresholded;
        cv::inRange(imgHSV, cv::Scalar(0, 10, 10), cv::Scalar(25, 255, 255), lower_red_hue_range); //Threshold the image
        cv::inRange(imgHSV, cv::Scalar(150, 10, 10), cv::Scalar(179, 255, 255), upper_red_hue_range);
        cv::addWeighted(lower_red_hue_range, 1.0, upper_red_hue_range, 1.0, 0.0, imgThresholded);

        // инвертировать маску, чтобы удалить все красные области
        imgThresholded = 255 - imgThresholded;
        cv::Mat gray;
        cv::cvtColor(imgThresholded,gray,CV_GRAY2BGR);
        cv::bitwise_and(result, gray, result);

        // закрыть все дыры, созданные маской
        cv::Mat const structure_elem = cv::getStructuringElement(
                             cv::MORPH_ELLIPSE, cv::Size(option_close_kernel_size, option_close_kernel_size));
        cv::morphologyEx(result, result, cv::MORPH_CLOSE, structure_elem);
        cv::Mat black_areas;
        cv::inRange(result, cv::Scalar(0,0,0), cv::Scalar(0,0,0), black_areas);
        //result = imgThresholded;
        result.setTo(cv::Scalar(0,0,255), black_areas); // снова сделайте отверстия красными!
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

                // полностью красный
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

                    // оба значения существуют
                    if (pixel_after){
                        new_value = (bgrPixel_before.val[0] < bgrPixel_after.val[0]) ? bgrPixel_before : bgrPixel_after;
                        result.at<cv::Vec3b>(j, i) = new_value;
                    }
                    // до того, как существует ценность
                    /*else if (j>0){
                        new_value = bgrPixel_before;
                        result.at<cv::Vec3b>(j, i) = new_value;
                    }*/



                }

                // сделайте что-нибудь со значениями BGR...
            }
        }
    }


    output_img = result;
    updateLabel();
}

void DepthRedefineDisparity::fillUpHoles(){
    cv::Mat result = input_img.clone();
    result.convertTo(result, CV_32FC1);
    const int size_u = 15, size_v = 15;
    const int size_s = lightfield_img.cols/size_u;
    const int size_t = lightfield_img.rows/size_v;

    // Настройте прямоугольник, чтобы определить интересующую вас область.
    cv::Rect myROI(size_s*(size_u/2), size_t*(size_v/2), size_s, size_t);

    // Обрежьте полное изображение до изображения, содержащегося в прямоугольнике myROI.
    // Обратите внимание, что это не копирует данные
    cv::Mat center_img;
    center_img = lightfield_img(myROI).clone();
    center_img.convertTo(center_img, CV_32FC3);

    // Распространение на глубину
    cv::Mat final_depth( size_t, size_s, CV_32FC1);

    int np = 3; // радиус для поиска лучшей цены
    int pixels_empty = 1; // давайте предположим, что хотя бы 1 пиксель неправильный

    // заполнить все пиксели!
    while(pixels_empty != 0){
        pixels_empty = 0;
        for (int t=0; t < size_t; t++){
        for (int s=0; s < size_s; s++){
            float current_depth = result.at<float>(t, s);
            // Заполняйте только пустые значения глубины
            if (current_depth == 0){
                float best_appr = 0;
                cv::Vec3f c_col = center_img.at<cv::Vec3f>(t, s);
                float col_dist_neighbour = 999999;

                // Обыскать всех соседей по выгодной цене
                for( int py=-np; py < np; py++){
                for( int px=-np; px < np; px++){
                    if (py==0 && px == 0)
                        continue;
                    int y = t+py;
                    int x = s+px;
                    y = std::max(0, std::min(y, size_t-1));
                    x = std::max(0, std::min(x, size_s-1));
                    float neighbour = result.at<float>(y, x);

                    // найдено хорошее соотношение цены и качества -> поиск лучшего значения
                    if (neighbour > 0){
                        cv::Vec3f n_col = center_img.at<cv::Vec3f>(y, x);
                        if (cv::norm(c_col - n_col) < col_dist_neighbour){
                            col_dist_neighbour = cv::norm(c_col - n_col);
                            best_appr = neighbour;
                        }
                    }
                }}
                if (best_appr == 0) // хорошего решения не найдено? еще раз в следующий раз!
                    pixels_empty++;
                else
                    final_depth.at<float>(t, s) = best_appr;
            }
            else
                final_depth.at<float>(t, s) = current_depth;
        }}
        result = final_depth.clone();
    }
    output_img = result.clone();
    updateLabel();
}
