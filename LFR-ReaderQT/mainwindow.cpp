#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lfp_reader.h"
#include "lfp_raw_view.h"
#include "myqgraphicsview.h"

#include <fstream>

#include <QDebug>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
#include <QDate>
#include <QMessageBox>
#include <QComboBox>
#include <QTextStream>

#include "reconstruction3d.h"
#include "imagedepth.h"
#include "depthfromfocus.h"
#include "depthcostaware.h"
#include "calibration.h"
#include "depthredefinedisparity.h"
#include "depthstereolike.h"
#include "depthfromepipolarimages.h"
#include "depthstereo.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QHBoxLayout* h_layout = new QHBoxLayout();
    tabWidget = new QTabWidget();
    h_layout->addWidget(tabWidget);

    ui->centralWidget->setLayout(h_layout);

    connect(ui->actionLFP, &QAction::triggered, this, &MainWindow::chooseLFP);
    connect(ui->actionLFImage, &QAction::triggered, this, &MainWindow::chooseLFImage);
    connect(ui->actionConvert_Images, &QAction::triggered, this, &MainWindow::chooseConvertImages);

    connect(ui->actionExtractLFFolder, &QAction::triggered, this, &MainWindow::chooseExtractRawLFFolder);
    connect(ui->actionCreateFromPNGs, &QAction::triggered, this, &MainWindow::chooseCreateVideoFromPNGs);
    connect(ui->actionPlayer, &QAction::triggered, this, &MainWindow::chooseVideoPlayer);
    connect(ui->actionOpen_Image_Sequence, &QAction::triggered, this, &MainWindow::chooseVideoFromImageSequence);
    connect(ui->actionCreate_3DScene, &QAction::triggered, this, &MainWindow::chooseCreate3DScene);
    connect(ui->actionGenerate_DepthMap, &QAction::triggered, this, &MainWindow::chooseGenerate_DepthMap);

    connect(ui->actionCreateCalibrationImage, &QAction::triggered, this, &MainWindow::chooseCreateCalibrationImage);
    connect(ui->actionCho_EstimateCenterPositions, &QAction::triggered, this, &MainWindow::chooseCho_EstimateCenterPositions);

    //chooseLFImage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chooseCreateCalibrationImage(){
    QStringList files = QFileDialog::getOpenFileNames(this,
                               QString("Choose all RAW Images"), // window name
                               "../",                            // directory
                               QString("Extracted RAW Images(*.PNG)"));   // filetype

    if (files.length() > 0){

        // first try to read meta info
        QString filename = files[0].split('.')[0];
        std::string txt_file = filename.toStdString() + ".TXT";

        if (QFile(QString::fromStdString(txt_file)).exists()){
            std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
            std::string text = reader.readText(input);
            QString meta_info = QString::fromStdString(text);
            //addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
            reader.parseLFMetaInfo(meta_info);
        }
        else{
            int ret = QMessageBox::warning(this, tr("Loading LightField Image"),
                                 tr("Meta Info File not found.\n"
                                    "Do you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
        }

        //Create average image
        //cv::Mat image = calibration::createCalibrationImage(files);
        //cv::imwrite("calibrated_img.png", image);

        //Create Demosaiced image
        //cv::Mat demosaiced_image = calibration::demosaicCalibrationImage(this, QString("calibrated_img.png"), reader.meta_infos, 3);
        //cv::imwrite("calibrated_img_demosaiced.png", demosaiced_image);

        //Convert to grayscale
        cv::Mat demosaiced_image = cv::imread(files[0].toStdString(), 1);
        cv::Mat grayscaled_image;
        cv::cvtColor(demosaiced_image, grayscaled_image, cv::COLOR_BGR2GRAY);
        cv::normalize(grayscaled_image, grayscaled_image, 0, 255, CV_MINMAX );
        cv::imwrite("calibrated_stretched.png", grayscaled_image);


        //cv::imwrite("calibrated_img_demosaiced.png", demosaiced_image);

    }
}

void MainWindow::chooseCho_EstimateCenterPositions(){
    // Display dialog so the user can select a file
    QString filename = QFileDialog::getOpenFileName(this,
                                            QString("Open White Image"),
                                            QDir::currentPath(),
                                            QString("White Image(*.PNG *.JPG)"));

    if (!filename.isEmpty()){

        float rotationAngle = calibration::cho_EstimateRotation(filename);
        qDebug() << "Rotation Angle: " << rotationAngle;
    }
}

void MainWindow::chooseExtractRawLFFolder(){
    QStringList files = QFileDialog::getOpenFileNames(this,
                               QString("Choose multiple Lightfield Images"), // window name
                               "../",                                       // directory
                               QString("LightField Container(*.LFP *.LFR *.RAW)"));   // filetype
    for(int i=0; i<files.length(); i++){
        QString file = files[i];
        QString output_name(QDir::currentPath() + "/RAW_" + QDate::currentDate().toString(Qt::ISODate)
                            + "_" + QString::number(i) + ".png");
        if (file.endsWith("lfp", Qt::CaseInsensitive)
                || file.endsWith("lfr", Qt::CaseInsensitive)){
            reader.read_lfp(this, files[i].toStdString(), output_name.toStdString());
        }
        // Loading RAW + txt (optional)
        else if (file.endsWith("RAW", Qt::CaseInsensitive)){

            // first try to read meta info
            QString filename = file.split('.')[0];
            std::string txt_file = filename.toStdString() + ".TXT";

            if (QFile(QString::fromStdString(txt_file)).exists()){
                std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
                std::string text = reader.readText(input);
                QString meta_info = QString::fromStdString(text);
                //addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
                reader.parseLFMetaInfo(meta_info);
                if (!output_name.isEmpty()){
                    QString output_text_file = output_name;
                    output_text_file.chop(4);
                    QFile outputFile(output_text_file + ".txt");
                    outputFile.open(QIODevice::WriteOnly);
                    // Check it opened OK
                    if(!outputFile.isOpen()){
                        qDebug() << "- Error, unable to open" << output_text_file << "for output";
                        //return 1;
                    }
                    outputFile.write(text.c_str(), text.length());        // write to stderr
                    outputFile.close();
                }
            }

            // now reading raw file
            reader.read_RAWFile(this, file.toStdString(), output_name.toStdString());
        }
    }
}

void MainWindow::chooseVideoPlayer(){
    // Display dialog so the user can select a file
    QString filename = QFileDialog::getOpenFileName(this,
                                            tr("Open Video"),
                                            QDir::currentPath(),
                                            tr("Files (*.*)"));

    if (filename.isEmpty()) // Do nothing if filename is empty
        return;

    // first try to read meta info
    QString metafile = filename.split('.')[0];
    std::string txt_file = metafile.toStdString() + ".TXT";

    if (QFile(QString::fromStdString(txt_file)).exists()){
        std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
        std::string text = reader.readText(input);
        QString meta_info = QString::fromStdString(text);
        //addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
        reader.parseLFMetaInfo(meta_info);
    }
    else{
        int ret = QMessageBox::warning(this, tr("Loading LightField Video"),
                             tr("Meta Info File not found.\n"
                                "Do you want to continue?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }
    tabWidget->clear();

    QHBoxLayout* view_layout = new QHBoxLayout();
    QWidget* view_widget = new QWidget();
    view_widget->setLayout(view_layout);
    QOpenGL_LFViewer *opengl_movieplayer = new QOpenGL_LFViewer(this, filename, true, reader.meta_infos);
    view_layout->addWidget(opengl_movieplayer);
    //opengl_movieplayer->update();
    tabWidget->addTab(view_widget,"View");

    // Color Tab
    /////////////////
    QGridLayout* buttons_layout = new QGridLayout();
    QWidget* buttons_widget = new QWidget();
    buttons_widget->setMaximumSize(300,1000);
    buttons_widget->setLayout(buttons_layout);

    QPushButton *gray = new QPushButton("Gray");
    QPushButton *bayer = new QPushButton("Bayer");
    QPushButton *demosaic = new QPushButton("Demosaic");
    connect( gray, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonGrayClicked()) );
    connect( bayer, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonBayerClicked()) );
    connect( demosaic, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonDemosaicClicked()) );
    buttons_layout->addWidget(gray,0,1);
    buttons_layout->addWidget(bayer,0,2);
    buttons_layout->addWidget(demosaic,1,1);

    // Color correction Tab
    QWidget* colorcorrect_options = new QWidget();
    QVBoxLayout* colorcorrect_options_layout = new QVBoxLayout();
    colorcorrect_options->setLayout(colorcorrect_options_layout);
    QCheckBox *checkWhiteBalance = new QCheckBox("WhiteBalance");
    QCheckBox *checkCCM = new QCheckBox("CCM");
    QCheckBox *checkGamma = new QCheckBox("Gamma");
    connect( checkWhiteBalance, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleWhiteBalance(bool)) );
    connect( checkCCM, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleCCM(bool)) );
    connect( checkGamma, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleGamma(bool)) );
    checkWhiteBalance->setChecked(true);
    checkCCM->setChecked(true);
    checkGamma->setChecked(true);
    colorcorrect_options_layout->addWidget(checkWhiteBalance);
    colorcorrect_options_layout->addWidget(checkCCM);
    colorcorrect_options_layout->addWidget(checkGamma);
    buttons_layout->addWidget(colorcorrect_options,1,2);

    QWidget* display_options = new QWidget();
    QVBoxLayout* display_options_layout = new QVBoxLayout();
    display_options->setLayout(display_options_layout);
    QPushButton *display = new QPushButton("Display");
    QSlider* focus_slider = new QSlider(Qt::Vertical, this);
    connect( focus_slider, SIGNAL(valueChanged(int)), opengl_movieplayer, SLOT(focus_changed(int)));
    connect( display, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonDisplayClicked()) );
    focus_slider->setMaximum(250);
    focus_slider->setMinimum(-250);
    QPushButton *start_pause = new QPushButton("Start/Pause");
    connect( start_pause, SIGNAL(clicked()), opengl_movieplayer, SLOT(start_video()) );
    QPushButton *stop = new QPushButton("Stop");
    connect( stop, SIGNAL(clicked()), opengl_movieplayer, SLOT(stop_video()) );
    //QCheckBox *checkSuperResolution = new QCheckBox("SuperResolution");
    //connect( checkSuperResolution, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleSuperResolution(bool)) );
    //checkSuperResolution->setChecked(true);
    //display_options_layout->addWidget(checkSuperResolution);
    QCheckBox *demosaic_render = new QCheckBox("Demosaic");
    connect( demosaic_render, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(renderDemosaic(bool)) );
    demosaic_render->setChecked(false);
    QLabel *fps_display = new QLabel("Fps: ");
    connect( opengl_movieplayer, SIGNAL(refreshFPS(QString)), fps_display, SLOT(setText(QString)));
    display_options_layout->addWidget(fps_display);
    display_options_layout->addWidget(demosaic_render);
    display_options_layout->addWidget(display);
    display_options_layout->addWidget(start_pause);
    display_options_layout->addWidget(stop);
    display_options_layout->addWidget(focus_slider);
    buttons_layout->addWidget(display_options,2,1);

    view_layout->addWidget(buttons_widget);
}

void MainWindow::chooseVideoFromImageSequence(){
    // Display dialog so the user can select a file
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          QString("Choose multiple Lightfield Images"), // window name
                                                          "../",                                       // directory
                                                          QString("LightField Images(*.PNG *.JPG)"));   // filetype

    if (filenames.isEmpty()) // Do nothing if filename is empty
        return;

    // first try to read meta info
    QString metafile = filenames.at(0).split('.')[0];
    std::string txt_file = metafile.toStdString() + ".TXT";

    if (QFile(QString::fromStdString(txt_file)).exists()){
        std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
        std::string text = reader.readText(input);
        QString meta_info = QString::fromStdString(text);
        //addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
        reader.parseLFMetaInfo(meta_info);
    }
    else{
        int ret = QMessageBox::warning(this, tr("Loading LightField Video"),
                             tr("Meta Info File not found.\n"
                                "Do you want to continue?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::No);
        if (ret == QMessageBox::No)
            return;
    }
    tabWidget->clear();

    QHBoxLayout* view_layout = new QHBoxLayout();
    QWidget* view_widget = new QWidget();
    view_widget->setLayout(view_layout);
    QOpenGL_LFViewer *opengl_movieplayer = new QOpenGL_LFViewer(this, filenames, true, reader.meta_infos);
    view_layout->addWidget(opengl_movieplayer);
    //opengl_movieplayer->update();
    tabWidget->addTab(view_widget,"View");

    // Color Tab
    /////////////////
    QGridLayout* buttons_layout = new QGridLayout();
    QWidget* buttons_widget = new QWidget();
    buttons_widget->setMaximumSize(300,1000);
    buttons_widget->setLayout(buttons_layout);

    QPushButton *gray = new QPushButton("Gray");
    QPushButton *bayer = new QPushButton("Bayer");
    QPushButton *demosaic = new QPushButton("Demosaic");
    connect( gray, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonGrayClicked()) );
    connect( bayer, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonBayerClicked()) );
    connect( demosaic, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonDemosaicClicked()) );
    buttons_layout->addWidget(gray,0,1);
    buttons_layout->addWidget(bayer,0,2);
    buttons_layout->addWidget(demosaic,1,1);

    // Color correction Tab
    QWidget* colorcorrect_options = new QWidget();
    QVBoxLayout* colorcorrect_options_layout = new QVBoxLayout();
    colorcorrect_options->setLayout(colorcorrect_options_layout);
    QCheckBox *checkWhiteBalance = new QCheckBox("WhiteBalance");
    QCheckBox *checkCCM = new QCheckBox("CCM");
    QCheckBox *checkGamma = new QCheckBox("Gamma");
    connect( checkWhiteBalance, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleWhiteBalance(bool)) );
    connect( checkCCM, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleCCM(bool)) );
    connect( checkGamma, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleGamma(bool)) );
    checkWhiteBalance->setChecked(true);
    checkCCM->setChecked(true);
    checkGamma->setChecked(true);
    colorcorrect_options_layout->addWidget(checkWhiteBalance);
    colorcorrect_options_layout->addWidget(checkCCM);
    colorcorrect_options_layout->addWidget(checkGamma);
    buttons_layout->addWidget(colorcorrect_options,1,2);

    QWidget* display_options = new QWidget();
    QVBoxLayout* display_options_layout = new QVBoxLayout();
    display_options->setLayout(display_options_layout);
    QPushButton *display = new QPushButton("Display");
    QSlider* focus_slider = new QSlider(Qt::Vertical, this);
    connect( focus_slider, SIGNAL(valueChanged(int)), opengl_movieplayer, SLOT(focus_changed(int)));
    connect( display, SIGNAL(clicked()), opengl_movieplayer, SLOT(buttonDisplayClicked()) );
    focus_slider->setMaximum(250);
    focus_slider->setMinimum(-250);
    QPushButton *start_pause = new QPushButton("Start/Pause");
    connect( start_pause, SIGNAL(clicked()), opengl_movieplayer, SLOT(start_video()) );
    QPushButton *stop = new QPushButton("Stop");
    connect( stop, SIGNAL(clicked()), opengl_movieplayer, SLOT(stop_video()) );
    //QCheckBox *checkSuperResolution = new QCheckBox("SuperResolution");
    //connect( checkSuperResolution, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(toggleSuperResolution(bool)) );
    //checkSuperResolution->setChecked(true);
    //display_options_layout->addWidget(checkSuperResolution);
    QCheckBox *demosaic_render = new QCheckBox("Demosaic");
    connect( demosaic_render, SIGNAL(toggled(bool)), opengl_movieplayer, SLOT(renderDemosaic(bool)) );
    demosaic_render->setChecked(false);
    QLabel *fps_display = new QLabel("Fps: ");
    connect( opengl_movieplayer, SIGNAL(refreshFPS(QString)), fps_display, SLOT(setText(QString)));
    display_options_layout->addWidget(fps_display);
    display_options_layout->addWidget(demosaic_render);
    display_options_layout->addWidget(display);
    display_options_layout->addWidget(start_pause);
    display_options_layout->addWidget(stop);
    display_options_layout->addWidget(focus_slider);
    buttons_layout->addWidget(display_options,2,1);

    view_layout->addWidget(buttons_widget);
}

void MainWindow::chooseCreateVideoFromPNGs(){
    QStringList files = QFileDialog::getOpenFileNames(this,
                            QString("Choose multiple Lightfield Images"), // window name
                            "../",                                       // directory
                            QString("LightField Images(*.PNG *.JPG)"));   // filetype


    if(!files.empty()){
        int image_type = QMessageBox::warning(this, tr("Image Format"),
                             tr("What kind of images?\n"
                                "Raw, Demosaiced or UV-ST?"),
                             "Raw", "Demosaiced", "UV-ST",0);

        std::string codec;
        if (image_type == 0) // raw
            codec = "Y800"; // Y800 works!
        else if (image_type == 1) // demosaicd // YV12, FFV1, MPEG, GREY
            codec = "HFYU";
        else if (image_type == 2) // UV-ST // MP42, I263, DIV3
            codec = "HFYU"; // Y422
        char c[4];
        strcpy(c, codec.c_str());

        // use first image as a reference image for the video
        QImage info(files.at(0));
        cv::Size S = cv::Size(static_cast<int>(info.width()), static_cast<int>(info.height()));
        std::string name = files[0].split(".")[0].toStdString() + ".avi";

        // Open the output
        cv::VideoWriter outputVideo;
        int fps = 5;
        if (image_type == 0)
            outputVideo.open(name, CV_FOURCC(c[0],c[1],c[2],c[3]), fps, S, false);
        else
            outputVideo.open(name, CV_FOURCC(c[0],c[1],c[2],c[3]), fps, S, true);

        if (!outputVideo.isOpened())
        {
            qDebug() << "Could not open the output video for writing " << QString::fromStdString(name) << endl;
            //qDebug() << "Perhaps wrong codec?!" << endl;
            return;
        }

        qDebug() << "Input frame resolution: Width=" << info.width() << "  Height=" << info.height()
             << endl;
        qDebug() << "Input codec type: " << QString::fromStdString(codec) << endl;

        cv::Mat src;
        for(int i=0;i<files.length();i++) //Show the image captured in the window and repeat
        {
            if (image_type == 0)
                src = cv::imread((files[i]).toStdString(), CV_LOAD_IMAGE_ANYDEPTH);
            else
                src = cv::imread((files[i]).toStdString(), CV_LOAD_IMAGE_COLOR);// read
            if (src.empty()){
                qDebug() << "src is empty \n file:" << files[i];
                break;         // check if at end
            }
            outputVideo.write(src);
        }
        qDebug() << "Finished writing" << endl;
    }
}

void MainWindow::chooseLFImage(){
    QString file = QFileDialog::getOpenFileName(this,
                               QString("Choose Lightfield Image"),           // window name
                               "../",                                       // relative folder
                               QString("LightField Images(*.PNG *.JPG)"));  // filetype
    if (!file.isNull()){
        ui->statusBar->showMessage("Loading Lightfield...");
        tabWidget->clear();

        // first try to read meta info
        QString filename = file.split('.')[0];
        std::string txt_file = filename.toStdString() + ".TXT";

        if (QFile(QString::fromStdString(txt_file)).exists()){
            std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
            std::string text = reader.readText(input);
            QString meta_info = QString::fromStdString(text);
            addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
            reader.parseLFMetaInfo(meta_info);
        }
        else{
            int ret = QMessageBox::warning(this, tr("Loading LightField Image"),
                                 tr("Meta Info File not found.\n"
                                    "Do you want to continue?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No);
            if (ret == QMessageBox::No)
                return;
        }

        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* view_widget = new QWidget();
        view_widget->setLayout(view_layout);
        QOpenGL_LFViewer *opengl_viewer = new QOpenGL_LFViewer(this, file, false, reader.meta_infos);
        view_layout->addWidget(opengl_viewer);
        tabWidget->addTab(view_widget,"View");

        // Color Tab
        /////////////////
        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton *gray = new QPushButton("Gray");
        QPushButton *bayer = new QPushButton("Bayer");
        connect( gray, SIGNAL(clicked()), opengl_viewer, SLOT(buttonGrayClicked()) );
        connect( bayer, SIGNAL(clicked()), opengl_viewer, SLOT(buttonBayerClicked()) );
        buttons_layout->addWidget(gray,0,1);
        buttons_layout->addWidget(bayer,0,2);

        QWidget* demosaic_options = new QWidget();
        QVBoxLayout* demosaic_options_layout = new QVBoxLayout();
        demosaic_options->setLayout(demosaic_options_layout);
        QPushButton *demosaic = new QPushButton("Demosaic");
        QSpinBox *spin_demosaic_mode = new QSpinBox();
        spin_demosaic_mode->setValue(1);
        spin_demosaic_mode->setMinimum(0);
        spin_demosaic_mode->setMaximum(2);
        demosaic_options_layout->addWidget(demosaic);
        demosaic_options_layout->addWidget(spin_demosaic_mode);
        connect( spin_demosaic_mode, SIGNAL(valueChanged(int)), opengl_viewer, SLOT(setDemosaicingMode(int)) );
        connect( demosaic, SIGNAL(clicked()), opengl_viewer, SLOT(buttonDemosaicClicked()) );
        buttons_layout->addWidget(demosaic_options,1,1);


        // Color correction Tab
        QWidget* colorcorrect_options = new QWidget();
        QVBoxLayout* colorcorrect_options_layout = new QVBoxLayout();
        colorcorrect_options->setLayout(colorcorrect_options_layout);
        QCheckBox *checkWhiteBalance = new QCheckBox("WhiteBalance");
        QCheckBox *checkCCM = new QCheckBox("CCM");
        QCheckBox *checkGamma = new QCheckBox("Gamma");
        connect( checkWhiteBalance, SIGNAL(toggled(bool)), opengl_viewer, SLOT(toggleWhiteBalance(bool)) );
        connect( checkCCM, SIGNAL(toggled(bool)), opengl_viewer, SLOT(toggleCCM(bool)) );
        connect( checkGamma, SIGNAL(toggled(bool)), opengl_viewer, SLOT(toggleGamma(bool)) );
        //connect( saveImage, SIGNAL(clicked()), opengl_viewer, SLOT(savePixmap()) );
        //connect( saveRaw, SIGNAL(clicked()), opengl_viewer, SLOT(saveRaw()) );
        checkWhiteBalance->setChecked(true);
        checkCCM->setChecked(true);
        checkGamma->setChecked(true);
        colorcorrect_options_layout->addWidget(checkWhiteBalance);
        colorcorrect_options_layout->addWidget(checkCCM);
        colorcorrect_options_layout->addWidget(checkGamma);
        buttons_layout->addWidget(colorcorrect_options,1,2);

        QDoubleSpinBox* rotation = new QDoubleSpinBox();
        rotation->setDecimals(15);
        rotation->setValue(reader.meta_infos.mla_rotation);
        connect( rotation, SIGNAL(valueChanged(double)), opengl_viewer, SLOT(setRotation(double)) );
        buttons_layout->addWidget(rotation,2,1);

        QWidget* display_options = new QWidget();
        QVBoxLayout* display_options_layout = new QVBoxLayout();
        QComboBox* decode_mode = new QComboBox();
        decode_mode->addItem("Meta");
        decode_mode->addItem("Dansereau");
        decode_mode->addItem("Cho");
        connect( decode_mode, SIGNAL(currentIndexChanged(int)), opengl_viewer, SLOT(setDecodeMode(int)) );
        display_options->setLayout(display_options_layout);
        QPushButton *uvmode = new QPushButton("UV-ST");
        connect( uvmode, SIGNAL(clicked()), opengl_viewer, SLOT(buttonUVModeClicked()) );
        QPushButton *display = new QPushButton("Display");
        connect( display, SIGNAL(clicked()), opengl_viewer, SLOT(buttonDisplayClicked()) );
        //QCheckBox *checkSuperResolution = new QCheckBox("SuperResolution");
        //connect( checkSuperResolution, SIGNAL(toggled(bool)), opengl_viewer, SLOT(toggleSuperResolution(bool)) );
        //checkSuperResolution->setChecked(true);
        //display_options_layout->addWidget(checkSuperResolution);
        QCheckBox *demosaic_render = new QCheckBox("Is demosaicked");
        connect( demosaic_render, SIGNAL(toggled(bool)), opengl_viewer, SLOT(renderDemosaic(bool)) );
        demosaic_render->setChecked(false);
        QCheckBox *render = new QCheckBox("Render frames");
        connect( render, SIGNAL(toggled(bool)), opengl_viewer, SLOT(renderFrames(bool)) );
        demosaic_render->setChecked(false);
        QLabel *fps_display = new QLabel("Fps: ");
        connect( opengl_viewer, SIGNAL(refreshFPS(QString)), fps_display, SLOT(setText(QString)));
        display_options_layout->addWidget(fps_display);
        display_options_layout->addWidget(decode_mode);
        display_options_layout->addWidget(demosaic_render);
        display_options_layout->addWidget(display);
        display_options_layout->addWidget(uvmode);
        display_options_layout->addWidget(render);
        buttons_layout->addWidget(display_options,3,1);

        QPushButton *saveImage = new QPushButton("SaveImage");
        QPushButton *saveRaw = new QPushButton("SaveRaw");

        connect( saveImage, SIGNAL(clicked()), opengl_viewer, SLOT(saveImage()) );
        connect( saveRaw, SIGNAL(clicked()), opengl_viewer, SLOT(saveRaw()) );
        buttons_layout->addWidget(saveImage,4,1);
        buttons_layout->addWidget(saveRaw,4,2);
        view_layout->addWidget(buttons_widget);
        opengl_viewer->update();

    }
}

void MainWindow::chooseConvertImages(){
    // Display dialog so the user can select a file
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          QString("Choose multiple Lightfield Images"), // window name
                                                          "../",                                       // directory
                                                          QString("LightField Images(*.PNG *.JPG)"));   // filetype

    if (filenames.isEmpty()) // Do nothing if filename is empty
        return;

    for(int i=0; i<filenames.length(); i++){
        QImage(filenames[i]).save(filenames[i].split('.')[0] + ".jpg");
    }
}

void MainWindow::chooseLFP(){

    QString file = QFileDialog::getOpenFileName(this,
                               QString("Choose Lightfield File"),           // window name
                               "../",                                       // relative folder
                               QString("LightFields(*.LFP *.LFR *.RAW *.TXT)"));  // filetype

    if (!file.isNull()){
        ui->statusBar->showMessage("Loading Lightfield...");
        tabWidget->clear();

        // Loading LFP lightfield container
        if (file.endsWith("lfp", Qt::CaseInsensitive)
                || file.endsWith("lfr", Qt::CaseInsensitive)){
            std::string file_string = file.toStdString();
            reader.read_lfp(this, file_string);
        }
        // Loading RAW + txt (optional)
        else if (file.endsWith("RAW", Qt::CaseInsensitive)){

            // first try to read meta info
            QString filename = file.split('.')[0];
            std::string txt_file = filename.toStdString() + ".TXT";

            if (QFile(QString::fromStdString(txt_file)).exists()){
                std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
                std::string text = reader.readText(input);
                QString meta_info = QString::fromStdString(text);
                addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
                reader.parseLFMetaInfo(meta_info);
            }

            // now reading raw file
            reader.read_RAWFile(this, file.toStdString());

        }

        // Loading TXT file (only meta infos)
        else if (file.endsWith("TXT", Qt::CaseInsensitive)){
            std::basic_ifstream<unsigned char> input(file.toStdString(), std::ifstream::binary);
            std::string text = reader.readText(input);
            QString meta_info = QString::fromStdString(text);
            addTabMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
            reader.parseLFMetaInfo(meta_info);
        }

        ui->statusBar->showMessage("Finished!", 2000);
    }
    else
        ui->statusBar->showMessage("No file selected/loaded!");
}


void MainWindow::addTabMetaInfos( QString header, QString sha1, int sec_length,
                                 QString metainfos, QString TabString){

    // Header infos
    QLabel* l_header = new QLabel("Header: " + header.toLatin1().toHex());
    QLabel* l_sha1 = new QLabel("Sha1: " + sha1);
    QLabel* l_infos = new QLabel("Bytes: " + QString::number(sec_length));
    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget(l_header);
    v_layout->addWidget(l_sha1);
    v_layout->addWidget(l_infos);

    // Meta infos inside textbrowser
    QTextBrowser* text_browser = new QTextBrowser();
    text_browser->setText(metainfos);
    v_layout->addWidget(text_browser);

    // Container widget
    QWidget* complete_widget = new QWidget();
    complete_widget->setLayout(v_layout);
    tabWidget->addTab(complete_widget, TabString);
}

void MainWindow::addTabImage(QString header, QString sha1, int sec_length, QImage image,
                           bool raw_image, LFP_Reader::lf_meta meta_infos){

    // Header infos
    QLabel* l_header = new QLabel("Header: " + header.toLatin1().toHex());
    QLabel* l_sha1 = new QLabel("Sha1: " + sha1);
    QLabel* l_infos = new QLabel("Bytes: " + QString::number(sec_length));
    QLabel* imagesize = new QLabel("Size: " + QString::number(image.width()) + "x" + QString::number(image.height()));


    // Display widget for image
    color_view = new lfp_raw_view(NULL, image, meta_infos);
    QWidget* color_widget = new QWidget();
    QHBoxLayout* color_layout = new QHBoxLayout();
    color_widget->setLayout(color_layout);
    color_layout->addWidget(color_view);
    QTabWidget* image_tab = new QTabWidget();
    image_tab->addTab(color_widget,"Color");

    if (raw_image){
        // Color Tab
        /////////////////
        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setLayout(buttons_layout);

        QPushButton *gray = new QPushButton("Gray");
        QPushButton *bayer = new QPushButton("Bayer");
        QPushButton *demosaic = new QPushButton("Demosaic");
        connect( gray, SIGNAL(clicked()), color_view, SLOT(buttonGrayClicked()) );
        connect( bayer, SIGNAL(clicked()), color_view, SLOT(buttonBayerClicked()) );
        connect( demosaic, SIGNAL(clicked()), color_view, SLOT(buttonDemosaicClicked()) );
        buttons_layout->addWidget(gray,0,1);
        buttons_layout->addWidget(bayer,0,2);
        buttons_layout->addWidget(demosaic,1,1);

        // Color correction Tab
        QWidget* colorcorrect_options = new QWidget();
        QVBoxLayout* colorcorrect_options_layout = new QVBoxLayout();
        colorcorrect_options->setLayout(colorcorrect_options_layout);

        QCheckBox *checkWhiteBalance = new QCheckBox("WhiteBalance");
        QCheckBox *checkCCM = new QCheckBox("CCM");
        QCheckBox *checkGamma = new QCheckBox("Gamma");
        QPushButton *colorcorrect = new QPushButton("ColorCorrect");
        QPushButton *saveImage = new QPushButton("SaveImage");
        QPushButton *saveRaw = new QPushButton("SaveRaw");
        connect( checkWhiteBalance, SIGNAL(toggled(bool)), color_view, SLOT(toggleWhiteBalance(bool)) );
        connect( checkCCM, SIGNAL(toggled(bool)), color_view, SLOT(toggleCCM(bool)) );
        connect( checkGamma, SIGNAL(toggled(bool)), color_view, SLOT(toggleGamma(bool)) );
        connect( colorcorrect, SIGNAL(clicked()), color_view, SLOT(buttonColorCorrectClicked()) );
        connect( saveImage, SIGNAL(clicked()), color_view, SLOT(savePixmap()) );
        connect( saveRaw, SIGNAL(clicked()), color_view, SLOT(saveRaw()) );
        checkWhiteBalance->setChecked(true);
        checkCCM->setChecked(true);
        checkGamma->setChecked(true);
        colorcorrect_options_layout->addWidget(checkWhiteBalance);
        colorcorrect_options_layout->addWidget(checkCCM);
        colorcorrect_options_layout->addWidget(checkGamma);
        colorcorrect_options_layout->addWidget(colorcorrect);
        buttons_layout->addWidget(colorcorrect_options,1,2);
        buttons_layout->addWidget(saveImage,2,1);
        buttons_layout->addWidget(saveRaw,2,2);
        color_layout->addWidget(buttons_widget);

        // Microlens tab
        /////////////////
        microlens_view = new lfp_raw_view(NULL, image, meta_infos);
        microlens_view->demosaic(3);
        QGridLayout* microlens_options_layout = new QGridLayout();
        QWidget* microlens_options_widget = new QWidget();
        microlens_options_widget->setLayout(microlens_options_layout);
        QDoubleSpinBox *centerx = new QDoubleSpinBox();
        QDoubleSpinBox *centery = new QDoubleSpinBox();
        QDoubleSpinBox *radiusx = new QDoubleSpinBox();
        QDoubleSpinBox *radiusy = new QDoubleSpinBox();
        QDoubleSpinBox *rotation = new QDoubleSpinBox();
        centerx->setRange(-100,100);
        centery->setRange(-100,100);
        centerx->setDecimals(5);
        centery->setDecimals(5);
        radiusx->setDecimals(5);
        radiusy->setDecimals(5);
        rotation->setDecimals(7);
        connect( centerx, SIGNAL(valueChanged(double)), microlens_view, SLOT(setLensletX(double)) );
        connect( centery, SIGNAL(valueChanged(double)), microlens_view, SLOT(setLensletY(double)) );
        connect( radiusx, SIGNAL(valueChanged(double)), microlens_view, SLOT(setLensletWidth(double)) );
        connect( radiusy, SIGNAL(valueChanged(double)), microlens_view, SLOT(setLensletHeight(double)) );
        connect( rotation, SIGNAL(valueChanged(double)), microlens_view, SLOT(setLensletRotation(double)) );
        centerx->setValue(0);
        centerx->setValue(meta_infos.mla_centerOffset_x / meta_infos.mla_pixelPitch);
        centery->setValue(meta_infos.mla_centerOffset_y / meta_infos.mla_pixelPitch);
        radiusx->setValue((meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_x);
        radiusy->setValue((meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch) * meta_infos.mla_scale_y);
        rotation->setValue(meta_infos.mla_rotation);
        microlens_options_layout->addWidget(new QLabel("Center X:"),0,1);
        microlens_options_layout->addWidget(centerx,0,2);
        microlens_options_layout->addWidget(new QLabel("Center Y:"),1,1);
        microlens_options_layout->addWidget(centery,1,2);
        microlens_options_layout->addWidget(new QLabel("Radius X:"),2,1);
        microlens_options_layout->addWidget(radiusx,2,2);
        microlens_options_layout->addWidget(new QLabel("Radius Y:"),3,1);
        microlens_options_layout->addWidget(radiusy,3,2);
        microlens_options_layout->addWidget(new QLabel("Rotation(CW):"),4,1);
        microlens_options_layout->addWidget(rotation,4,2);
        QHBoxLayout* microlens_layout = new QHBoxLayout();
        QWidget* microlens_widget = new QWidget();
        microlens_widget->setLayout(microlens_layout);
        microlens_layout->addWidget(microlens_view);
        microlens_layout->addWidget(microlens_options_widget);
        image_tab->addTab(microlens_widget,"Microlenses");
    }


    // Container widget
    QWidget* complete_widget = new QWidget();
    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget(l_header);
    v_layout->addWidget(l_sha1);
    v_layout->addWidget(l_infos);
    v_layout->addWidget(imagesize);
    v_layout->addWidget(image_tab);
    complete_widget->setLayout(v_layout);
    tabWidget->addTab(complete_widget, "Image");
}

void MainWindow::chooseCreate3DScene(){

    ui->statusBar->showMessage("Loading Depth Map...");
    tabWidget->clear();

    reconstruction3D* recon = new reconstruction3D();

    QWidget* display_options = new QWidget();
    QVBoxLayout* display_options_layout = new QVBoxLayout();
    display_options->setLayout(display_options_layout);
    QPushButton *pcfmdms = new QPushButton("Point Cloud From DepthMaps");
    connect( pcfmdms, SIGNAL(clicked()), recon, SLOT(calcPointCloudFromMultipleDepthMaps()) );
    QPushButton *appr_normals = new QPushButton("Approximate Normals");
    connect( appr_normals, SIGNAL(clicked()), recon, SLOT(approximateNormals()) );
    QPushButton *show_pointcloud = new QPushButton("Show PointCloud");
    connect( show_pointcloud, SIGNAL(clicked()), recon, SLOT(showPointCloud()) );
    QPushButton *save_pointcloud = new QPushButton("Save PointCloud");
    connect( save_pointcloud, SIGNAL(clicked()), recon, SLOT(saveCloudToPLY()) );
    QDoubleSpinBox *mesh_mu = new QDoubleSpinBox();
    connect( mesh_mu, SIGNAL(valueChanged(double)), recon, SLOT(setMeshMu(double)) );
    QPushButton *calc_mesh = new QPushButton("Calc Mesh");
    connect( calc_mesh, SIGNAL(clicked()), recon, SLOT(calculateMesh()) );
    QPushButton *show_mesh = new QPushButton("Show Mesh");
    connect( show_mesh, SIGNAL(clicked()), recon, SLOT(showMesh()) );
    //QLabel *fps_display = new QLabel("Fps: ");
    //connect( opengl_viewer, SIGNAL(refreshFPS(QString)), fps_display, SLOT(setText(QString)));
    display_options_layout->addWidget(pcfmdms);
    display_options_layout->addWidget(appr_normals);
    display_options_layout->addWidget(show_pointcloud);
    display_options_layout->addWidget(save_pointcloud);
    display_options_layout->addWidget(mesh_mu);
    display_options_layout->addWidget(calc_mesh);
    display_options_layout->addWidget(show_mesh);
    //display_options_layout->addWidget(uvmode);
    tabWidget->addTab(display_options, "Options");
}

void MainWindow::chooseGenerate_DepthMap(){

    tabWidget->clear();

    /////////////////////////////
    /// Depth Stereo Like
    ///
    {
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* stereo_like_widget = new QWidget();
        stereo_like_widget->setLayout(view_layout);
        MyGraphicsView* view = new MyGraphicsView(this);

        view_layout->addWidget(view);

        DepthStereoLike* dsl = new DepthStereoLike(view);

        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton* load = new QPushButton("Load Image");
        connect(load, SIGNAL(clicked()), dsl, SLOT(loadImage()));
        buttons_layout->addWidget(load,0,1);

        QDoubleSpinBox* sobel_scale = new QDoubleSpinBox();
        sobel_scale->setValue(2.0);
        QSpinBox* sobel_k_size = new QSpinBox();
        sobel_k_size->setSingleStep(2);
        sobel_k_size->setMaximum(31);
        sobel_k_size->setMinimum(1);
        sobel_k_size->setValue(3);
        QDoubleSpinBox* gauss_sigma = new QDoubleSpinBox();
        gauss_sigma->setValue(2.0);
        QSpinBox* gauss_kernel = new QSpinBox();
        gauss_kernel->setSingleStep(2);
        gauss_kernel->setValue(9);
        gauss_kernel->setMaximum(31);
        gauss_kernel->setMinimum(1);
        connect(sobel_scale, SIGNAL(valueChanged(double)), dsl, SLOT(setSobelScale(double)));
        connect(sobel_k_size, SIGNAL(valueChanged(int)), dsl, SLOT(setSobelKernel(int)));
        connect(gauss_sigma, SIGNAL(valueChanged(double)), dsl, SLOT(setGaussSigma(double)));
        connect(gauss_kernel, SIGNAL(valueChanged(int)), dsl, SLOT(setGaussKernel(int)));

        buttons_layout->addWidget(sobel_scale,3,1);
        buttons_layout->addWidget(sobel_k_size,3,2);
        buttons_layout->addWidget(gauss_sigma,4,1);
        buttons_layout->addWidget(gauss_kernel,4,2);

        view_layout->addWidget(buttons_widget);

        //tabWidget->addTab(view_widget,"Depth Stereo Taxonomy");
        tabWidget->addTab(stereo_like_widget,"Depth Stereo Like");
    }

    /////////////////////////////
    /// Depth From Focus
    ///
    {
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* from_focus_widget = new QWidget();
        from_focus_widget->setLayout(view_layout);
        MyGraphicsView* view = new MyGraphicsView(this);

        view_layout->addWidget(view);

        DepthFromFocus* dff = new DepthFromFocus(view);

        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton* load = new QPushButton("Load Image");
        connect(load, SIGNAL(clicked()), dff, SLOT(loadImage()));
        buttons_layout->addWidget(load,0,1);

        QCheckBox* check_max_focus = new QCheckBox("Use Max Focus");
        connect(check_max_focus, SIGNAL(clicked(bool)), dff, SLOT(setUseMaxFocus(bool)));
        check_max_focus->setChecked(false);
        buttons_layout->addWidget(check_max_focus,1,1);
        QCheckBox* check_max_variance = new QCheckBox("Use Max Variance");
        check_max_variance->setChecked(false);
        connect(check_max_variance, SIGNAL(clicked(bool)), dff, SLOT(setUseMaxVariance(bool)));
        buttons_layout->addWidget(check_max_variance,2,1);

        QDoubleSpinBox* max_focus_threshold = new QDoubleSpinBox();
        max_focus_threshold->setValue(5000.0);
        max_focus_threshold->setMaximum(10000.0);
        connect(max_focus_threshold, SIGNAL(valueChanged(double)), dff, SLOT(setFocusThreshold(double)));
        buttons_layout->addWidget(max_focus_threshold,1,2);

        QDoubleSpinBox* max_variance = new QDoubleSpinBox();
        max_variance->setValue(0.0);
        max_variance->setMaximum(50.0);
        connect(max_variance, SIGNAL(valueChanged(double)), dff, SLOT(setMaxVariance(double)));
        buttons_layout->addWidget(max_variance,2,2);

        QCheckBox* check_threshold = new QCheckBox("Use Threshold");
        check_threshold->setChecked(false);
        connect(check_threshold, SIGNAL(clicked(bool)), dff, SLOT(setUseThreshold(bool)));
        buttons_layout->addWidget(check_threshold,3,1);

        QDoubleSpinBox* threshold = new QDoubleSpinBox();
        threshold->setValue(16.0);
        threshold->setMaximum(255.0);
        connect(threshold, SIGNAL(valueChanged(double)), dff, SLOT(setThreshold(double)));
        buttons_layout->addWidget(threshold,3,2);


        view_layout->addWidget(buttons_widget);


        tabWidget->addTab(from_focus_widget,"Depth From Focus");
    }

    /////////////////////////////
    /// Depth Cost Aware
    ///

    {
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* cost_aware_widget = new QWidget();
        cost_aware_widget->setLayout(view_layout);
        MyGraphicsView* view = new MyGraphicsView(this);

        view_layout->addWidget(view);

        DepthCostAware* dca = new DepthCostAware(view);

        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton* load = new QPushButton("Load Image");
        connect(load, SIGNAL(clicked()), dca, SLOT(loadImage()));
        buttons_layout->addWidget(load,0,1);

        QCheckBox* show_center_color_image = new QCheckBox("Show Center Image");
        connect(show_center_color_image, SIGNAL(clicked(bool)), dca, SLOT(setShowCenterColorImage(bool)));
        buttons_layout->addWidget(show_center_color_image,1,1);


        QDoubleSpinBox* threshold = new QDoubleSpinBox();
        threshold->setValue(16.0);
        threshold->setMaximum(100.0);
        connect(threshold, SIGNAL(valueChanged(double)), dca, SLOT(setFocusThreshold(double)));
        buttons_layout->addWidget(threshold,1,2);

        QPushButton* calc_cons = new QPushButton("Calculate Consistency Volume");
        connect(calc_cons, SIGNAL(clicked()), dca, SLOT(calcConsistencyVolume()));
        buttons_layout->addWidget(calc_cons,2,1);

        QPushButton* calc_focus = new QPushButton("Calculate Focus Volume");
        connect(calc_focus, SIGNAL(clicked()), dca, SLOT(calcFocusVolume()));
        buttons_layout->addWidget(calc_focus,2,2);


        QCheckBox* check_consistency = new QCheckBox("Consistency Cue");
        check_consistency->setChecked(dca->getUseConsistency());
        connect(check_consistency, SIGNAL(clicked(bool)), dca, SLOT(setConsistency(bool)));
        buttons_layout->addWidget(check_consistency,3,1);
        QCheckBox* check_focus = new QCheckBox("Focus Cue");
        check_focus->setChecked(dca->getUseFocusCue());
        connect(check_focus, SIGNAL(clicked(bool)), dca, SLOT(setFocusCue(bool)));
        buttons_layout->addWidget(check_focus,3,2);
        QCheckBox* check_filter_focus_sml_0 = new QCheckBox("filter Focus SML 0");
        connect(check_filter_focus_sml_0, SIGNAL(clicked(bool)), dca, SLOT(setFilterFocusSml0(bool)));
        check_filter_focus_sml_0->setChecked(dca->getFilterFocusSml0());
        buttons_layout->addWidget(check_filter_focus_sml_0,4,1);
        QCheckBox* check_filter_focus_bound = new QCheckBox("filter Focus Bound");
        check_filter_focus_bound->setChecked(dca->getFilterFocusBound());
        connect(check_filter_focus_bound, SIGNAL(clicked(bool)), dca, SLOT(setFilterFocusBound(bool)));
        buttons_layout->addWidget(check_filter_focus_bound,4,2);
        QCheckBox* check_filter_cons_variance = new QCheckBox("filter Cons Variance");
        check_filter_cons_variance->setChecked(dca->getFilterConsVariance());
        connect(check_filter_cons_variance, SIGNAL(clicked(bool)), dca, SLOT(setFilterConsVariance(bool)));
        buttons_layout->addWidget(check_filter_cons_variance,5,1);
        QDoubleSpinBox* max_variance = new QDoubleSpinBox();
        max_variance->setValue(10.0);
        max_variance->setMaximum(1000.0);
        connect(max_variance, SIGNAL(valueChanged(double)), dca, SLOT(setMaxVariance(double)));
        buttons_layout->addWidget(max_variance,5,2);
        QCheckBox* check_fill_holes = new QCheckBox("fill up holes");
        check_fill_holes->setChecked(dca->getFillUpHoles());
        connect(check_fill_holes, SIGNAL(clicked(bool)), dca, SLOT(setFillUpHoles(bool)));
        buttons_layout->addWidget(check_fill_holes,6,1);

        view_layout->addWidget(buttons_widget);

        //tabWidget->addTab(view_widget,"Depth Stereo Taxonomy");
        tabWidget->addTab(cost_aware_widget,"Depth Cost Aware");
    }

    /////////////////////////////
    /// Depth From Epipolar Images
    ///
    {
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* stereo_like_widget = new QWidget();
        stereo_like_widget->setLayout(view_layout);
        MyGraphicsView* view = new MyGraphicsView(this);

        view_layout->addWidget(view);

        DepthFromEpipolarImages* dfei = new DepthFromEpipolarImages(view);

        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton* load = new QPushButton("Load Image");
        connect(load, SIGNAL(clicked()), dfei, SLOT(loadImage()));
        buttons_layout->addWidget(load,0,1);

        QDoubleSpinBox* sobel_scale = new QDoubleSpinBox();
        sobel_scale->setValue(2.0);
        QSpinBox* sobel_k_size = new QSpinBox();
        sobel_k_size->setSingleStep(2);
        sobel_k_size->setMaximum(31);
        sobel_k_size->setMinimum(1);
        sobel_k_size->setValue(3);
        QDoubleSpinBox* gauss_sigma = new QDoubleSpinBox();
        gauss_sigma->setValue(2.0);
        QSpinBox* gauss_kernel = new QSpinBox();
        gauss_kernel->setSingleStep(2);
        gauss_kernel->setValue(9);
        gauss_kernel->setMaximum(31);
        gauss_kernel->setMinimum(1);
        connect(sobel_scale, SIGNAL(valueChanged(double)), dfei, SLOT(setSobelScale(double)));
        connect(sobel_k_size, SIGNAL(valueChanged(int)), dfei, SLOT(setSobelKernel(int)));
        connect(gauss_sigma, SIGNAL(valueChanged(double)), dfei, SLOT(setGaussSigma(double)));
        connect(gauss_kernel, SIGNAL(valueChanged(int)), dfei, SLOT(setGaussKernel(int)));

        buttons_layout->addWidget(sobel_scale,1,1);
        buttons_layout->addWidget(sobel_k_size,1,2);
        buttons_layout->addWidget(gauss_sigma,2,1);
        buttons_layout->addWidget(gauss_kernel,2,2);

        view_layout->addWidget(buttons_widget);

        //tabWidget->addTab(view_widget,"Depth From EPIs");
        tabWidget->addTab(stereo_like_widget,"Depth From EPIs");
    }



    /////////////////////////////
    /// Redefine disparity map (errors)
    ///
    {
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* redefine_disparity_widget = new QWidget();
        redefine_disparity_widget->setLayout(view_layout);
        MyGraphicsView* view = new MyGraphicsView(this);

        view_layout->addWidget(view);

        DepthRedefineDisparity* dff = new DepthRedefineDisparity(view);

        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setMaximumSize(300,1000);
        buttons_widget->setLayout(buttons_layout);

        QPushButton* load = new QPushButton("Load Image");
        connect(load, SIGNAL(clicked()), dff, SLOT(loadImage()));
        buttons_layout->addWidget(load,0,1);

        QPushButton* process = new QPushButton("Process");
        connect(process, SIGNAL(clicked()), dff, SLOT(fillInHolesInDepth()));
        buttons_layout->addWidget(process,0,2);

        QCheckBox* check_small_holes = new QCheckBox("Small holes");
        check_small_holes->setChecked(true);
        connect(check_small_holes, SIGNAL(clicked(bool)), dff, SLOT(setOptionSmallHoles(bool)));
        buttons_layout->addWidget(check_small_holes,2,1);

        QSpinBox* kernel_median = new QSpinBox();
        kernel_median->setValue(3);
        kernel_median->setSingleStep(2);
        kernel_median->setMinimum(1);
        kernel_median->setMaximum(21);
        connect(kernel_median, SIGNAL(valueChanged(int)), dff, SLOT(setMedianKernel(int)));
        buttons_layout->addWidget(kernel_median,2,2);

        QCheckBox* check_middle_holes = new QCheckBox("Middle holes");
        check_middle_holes->setChecked(true);
        connect(check_middle_holes, SIGNAL(clicked(bool)), dff, SLOT(setOptionMiddleHoles(bool)));
        buttons_layout->addWidget(check_middle_holes,3,1);

        QSpinBox* kernel = new QSpinBox();
        kernel->setValue(10);
        kernel->setSingleStep(1);
        kernel->setMinimum(2);
        kernel->setMaximum(20);
        connect(kernel, SIGNAL(valueChanged(int)), dff, SLOT(setCloseKernel(int)));
        buttons_layout->addWidget(kernel,3,2);

        QCheckBox* check_big_holes = new QCheckBox("Big holes");
        check_big_holes->setChecked(true);
        connect(check_big_holes, SIGNAL(clicked(bool)), dff, SLOT(setOptionBigHoles(bool)));
        buttons_layout->addWidget(check_big_holes,4,1);


        QSpinBox* big_holes_max_size = new QSpinBox();
        big_holes_max_size->setValue(50);
        big_holes_max_size->setSingleStep(1);
        big_holes_max_size->setMinimum(2);
        big_holes_max_size->setMaximum(1000);
        connect(big_holes_max_size, SIGNAL(valueChanged(int)), dff, SLOT(setBigHolesMaxSize(int)));
        buttons_layout->addWidget(big_holes_max_size,4,2);

        view_layout->addWidget(buttons_widget);


        tabWidget->addTab(redefine_disparity_widget,"Redefine Disparity");
    }

    /*
    QComboBox* cb = new QComboBox(this);
    cb->addItem("input");
    cb->addItem("sobel");
    cb->addItem("gauss*sobel");
    cb->addItem("disparity");
    cb->addItem("depthmap");
    cb->addItem("epi_uvst");
    cb->addItem("depthmap_uvst");
    connect(cb, SIGNAL(currentIndexChanged(QString)), id, SLOT(setViewMode(QString)));
    buttons_layout->addWidget(cb,2,1);

    QDoubleSpinBox* sobel_scale = new QDoubleSpinBox();
    sobel_scale->setValue(2.0);
    QSpinBox* sobel_k_size = new QSpinBox();
    sobel_k_size->setSingleStep(2);
    sobel_k_size->setMaximum(31);
    sobel_k_size->setMinimum(1);
    sobel_k_size->setValue(3);
    QDoubleSpinBox* gauss_sigma = new QDoubleSpinBox();
    gauss_sigma->setValue(2.0);
    QSpinBox* gauss_kernel = new QSpinBox();
    gauss_kernel->setSingleStep(2);
    gauss_kernel->setValue(9);
    gauss_kernel->setMaximum(31);
    gauss_kernel->setMinimum(1);
    connect(sobel_scale, SIGNAL(valueChanged(double)), id, SLOT(setSobelScale(double)));
    connect(sobel_k_size, SIGNAL(valueChanged(int)), id, SLOT(setSobelKernel(int)));
    connect(gauss_sigma, SIGNAL(valueChanged(double)), id, SLOT(setGaussSigma(double)));
    connect(gauss_kernel, SIGNAL(valueChanged(int)), id, SLOT(setGaussKernel(int)));

    buttons_layout->addWidget(sobel_scale,3,1);
    buttons_layout->addWidget(sobel_k_size,3,2);
    buttons_layout->addWidget(gauss_sigma,4,1);
    buttons_layout->addWidget(gauss_kernel,4,2);
    */
}
