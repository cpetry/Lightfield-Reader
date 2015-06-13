#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lfp_reader.h"
#include "myqgraphicsview.h"

#include <fstream>

#include <QDebug>
#include <QSpinBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
#include <QDate>
#include <QMessageBox>

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
    connect(ui->actionExtractLFFolder, &QAction::triggered, this, &MainWindow::chooseExtractRawLFFolder);
    connect(ui->actionCreateFromPNGs, &QAction::triggered, this, &MainWindow::chooseCreateVideoFromPNGs);
    connect(ui->actionPlayer, &QAction::triggered, this, &MainWindow::chooseVideoPlayer);
    connect(ui->actionOpen_Image_Sequence, &QAction::triggered, this, &MainWindow::chooseVideoFromImageSequence);

    //chooseLFImage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chooseExtractRawLFFolder(){
    QStringList files = QFileDialog::getOpenFileNames(this,
                               QString("Choose multiple Lightfield Images"), // window name
                               "../",                                       // directory
                               QString("LightField Container(*.LFP *.LFR *.RAW)"));   // filetype
    for(int i=0; i<files.length(); i++){
        QString output_name(QDir::currentPath() + "/raw_lfp_" + QDate::currentDate().toString(Qt::ISODate)
                            + "_" + QString::number(i) + ".png");
        reader.read_lfp(this, files[i].toStdString(), true, output_name.toStdString());
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
        QPushButton *demosaic = new QPushButton("Demosaic");
        connect( gray, SIGNAL(clicked()), opengl_viewer, SLOT(buttonGrayClicked()) );
        connect( bayer, SIGNAL(clicked()), opengl_viewer, SLOT(buttonBayerClicked()) );
        connect( demosaic, SIGNAL(clicked()), opengl_viewer, SLOT(buttonDemosaicClicked()) );
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

        QWidget* display_options = new QWidget();
        QVBoxLayout* display_options_layout = new QVBoxLayout();
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
        QLabel *fps_display = new QLabel("Fps: ");
        connect( opengl_viewer, SIGNAL(refreshFPS(QString)), fps_display, SLOT(setText(QString)));
        display_options_layout->addWidget(fps_display);
        display_options_layout->addWidget(demosaic_render);
        display_options_layout->addWidget(display);
        display_options_layout->addWidget(uvmode);
        buttons_layout->addWidget(display_options,2,1);

        QPushButton *saveImage = new QPushButton("SaveImage");
        QPushButton *saveRaw = new QPushButton("SaveRaw");

        connect( saveImage, SIGNAL(clicked()), opengl_viewer, SLOT(saveImage()) );
        connect( saveRaw, SIGNAL(clicked()), opengl_viewer, SLOT(saveRaw()) );
        buttons_layout->addWidget(saveImage,3,1);
        buttons_layout->addWidget(saveRaw,3,2);
        view_layout->addWidget(buttons_widget);
        opengl_viewer->update();

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
    color_view = new MyGraphicsView(NULL, image, meta_infos);
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
        microlens_view = new MyGraphicsView(NULL, image, meta_infos);
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

