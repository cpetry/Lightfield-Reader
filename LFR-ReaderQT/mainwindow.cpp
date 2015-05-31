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
    chooseLFImage();
}

MainWindow::~MainWindow()
{
    delete ui;
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

        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* view_widget = new QWidget();
        view_widget->setLayout(view_layout);
        opengl_viewer = new QOpenGL_LFViewer(this, QImage(file), reader.meta_infos);
        view_layout->addWidget(opengl_viewer);
        opengl_viewer->update();
        tabWidget->addTab(view_widget,"View");
    }
}

void MainWindow::chooseLFP(){

    QString file = QFileDialog::getOpenFileName(this,
                               QString("Choose Lightfield File"),           // window name
                               "../",                                       // relative folder
                               QString("LightFields(*.LFP *.RAW *.TXT)"));  // filetype

    if (!file.isNull()){
        ui->statusBar->showMessage("Loading Lightfield...");
        tabWidget->clear();

        // Loading LFP lightfield container
        if (file.endsWith("lfp", Qt::CaseInsensitive))
            reader.read_lfp(this, file.toStdString());

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

        // View tab
        /////////////////
        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* view_widget = new QWidget();
        view_widget->setLayout(view_layout);
        opengl_viewer = new QOpenGL_LFViewer(this, microlens_view->getFinishedImage(), meta_infos);
        view_layout->addWidget(opengl_viewer);
        opengl_viewer->update();

        QWidget* view_options_widget = new QWidget();
        QGridLayout* view_options_layout = new QGridLayout();
        view_options_widget->setLayout(view_options_layout);
        QDoubleSpinBox *overlap = new QDoubleSpinBox();
        overlap->setDecimals(7);
        overlap->setValue((meta_infos.mla_lensPitch / meta_infos.mla_pixelPitch));
        connect( overlap, SIGNAL(valueChanged(double)), opengl_viewer, SLOT(setOverlap(double)) );
        view_options_layout->addWidget(new QLabel("Overlap:"),0,1);
        view_options_layout->addWidget(overlap,0,2);
        image_tab->addTab(view_widget,"View");
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

