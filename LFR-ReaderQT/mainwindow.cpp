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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QHBoxLayout* h_layout = new QHBoxLayout();
    tabWidget = new QTabWidget();
    h_layout->addWidget(tabWidget);


    ui->centralWidget->setLayout(h_layout);

    connect(ui->actionFile, &QAction::triggered, this, &MainWindow::chooseFile);
    //chooseFile();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chooseFile(){
    QString file = QFileDialog::getOpenFileName(this, QString("Choose Lightfield File"), "../", QString("LightFields(*.LFP *.RAW *.TXT)"));
    if (!file.isNull()){
        ui->statusBar->showMessage("Loading Lightfield...");
        tabWidget->clear();

        // Loading RAW-file + optional txt file
        if (file.endsWith("RAW", Qt::CaseInsensitive)){
            // first read meta info
            QString filename = file.split('.')[0];
            std::string txt_file = filename.toStdString() + ".TXT";
            std::basic_ifstream<unsigned char> input(txt_file, std::ifstream::binary);
            std::string text = reader.readText(input);
            QString meta_info = QString::fromStdString(text);
            setupMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
            reader.parseLFMetaInfo(meta_info);

            // now reading raw file
            reader.read_RAWFile(this, file.toStdString());

        }
        else if (file.endsWith("TXT", Qt::CaseInsensitive)){
            std::basic_ifstream<unsigned char> input(file.toStdString(), std::ifstream::binary);
            std::string text = reader.readText(input);
            QString meta_info = QString::fromStdString(text);
            setupMetaInfos("No Header", "No sha1", meta_info.length(), meta_info, "MetaInfo");
            reader.parseLFMetaInfo(meta_info);
        }
        else if (file.endsWith("lfp", Qt::CaseInsensitive))
            reader.read_lfp(this, file.toStdString());
        ui->statusBar->showMessage("Finished!", 2000);
    }
    else
        ui->statusBar->showMessage("No file selected/loaded!");
}


void MainWindow::setupMetaInfos( QString header, QString sha1, int sec_length, QString metainfos, QString TabString){
    QLabel* l_header = new QLabel("Header: " + header.toLatin1().toHex());
    QLabel* l_sha1 = new QLabel("Sha1: " + sha1);
    QLabel* l_infos = new QLabel("Bytes: " + QString::number(sec_length));

    QWidget* complete_widget = new QWidget();
    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget(l_header);
    v_layout->addWidget(l_sha1);
    v_layout->addWidget(l_infos);

    QTextBrowser* text_browser = new QTextBrowser();
    text_browser->setText(metainfos);
    v_layout->addWidget(text_browser);

    complete_widget->setLayout(v_layout);
    tabWidget->addTab(complete_widget, TabString);
}

void MainWindow::setupView(QString header, QString sha1, int sec_length, QImage image, bool raw_image){
    QLabel* l_header = new QLabel("Header: " + header.toLatin1().toHex());
    QLabel* l_sha1 = new QLabel("Sha1: " + sha1);
    QLabel* l_infos = new QLabel("Bytes: " + QString::number(sec_length));
    QLabel* imagesize = new QLabel("Size: " + QString::number(image.width()) + "x" + QString::number(image.height()));

    QTabWidget* image_tab = new QTabWidget();

    color_view = new MyGraphicsView(NULL, image);
    QWidget* color_widget = new QWidget();
    QHBoxLayout* color_layout = new QHBoxLayout();
    color_widget->setLayout(color_layout);
    color_layout->addWidget(color_view);
    image_tab->addTab(color_widget,"Color");

    if (raw_image){
        QGridLayout* buttons_layout = new QGridLayout();
        QWidget* buttons_widget = new QWidget();
        buttons_widget->setLayout(buttons_layout);
        QPushButton *gray = new QPushButton("Gray");
        QPushButton *bayer = new QPushButton("Bayer");
        QPushButton *demosaic = new QPushButton("Demosaic");

        QWidget* colorcorrect_options = new QWidget();
        QVBoxLayout* colorcorrect_options_layout = new QVBoxLayout();
        colorcorrect_options->setLayout(colorcorrect_options_layout);
        QCheckBox *checkWhiteBalance = new QCheckBox("checkWhiteBalance");
        QCheckBox *checkCCM = new QCheckBox("checkCCM");
        QCheckBox *checkGamma = new QCheckBox("checkGamma");
        QPushButton *colorcorrect = new QPushButton("ColorCorrect");
        QPushButton *save = new QPushButton("Save");
        checkWhiteBalance->setChecked(true);
        checkCCM->setChecked(true);
        checkGamma->setChecked(true);
        connect( gray, SIGNAL(clicked()), color_view, SLOT(buttonGrayClicked()) );
        connect( bayer, SIGNAL(clicked()), color_view, SLOT(buttonBayerClicked()) );
        connect( demosaic, SIGNAL(clicked()), color_view, SLOT(buttonDemosaicClicked()) );
        connect( checkWhiteBalance, SIGNAL(toggled(bool)), color_view, SLOT(toggleWhiteBalance(bool)) );
        connect( checkCCM, SIGNAL(toggled(bool)), color_view, SLOT(toggleCCM(bool)) );
        connect( checkGamma, SIGNAL(toggled(bool)), color_view, SLOT(toggleGamma(bool)) );
        connect( colorcorrect, SIGNAL(clicked()), color_view, SLOT(buttonColorCorrectClicked()) );
        connect( save, SIGNAL(clicked()), color_view, SLOT(buttonSaveClicked()) );
        buttons_layout->addWidget(gray,0,1);
        buttons_layout->addWidget(bayer,0,2);
        buttons_layout->addWidget(demosaic,1,1);
        colorcorrect_options_layout->addWidget(checkWhiteBalance);
        colorcorrect_options_layout->addWidget(checkCCM);
        colorcorrect_options_layout->addWidget(checkGamma);
        colorcorrect_options_layout->addWidget(colorcorrect);
        buttons_layout->addWidget(colorcorrect_options,1,2);
        buttons_layout->addWidget(save,2,2);
        color_layout->addWidget(buttons_widget);


        microlens_view = new MyGraphicsView(NULL, image);
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
        centerx->setValue(-7.3299 / 1.399);
        centery->setValue( 5.5686 / 1.399);
        radiusx->setValue(14.2959);
        radiusy->setValue(14.2959 * 1.0001299);
        rotation->setValue(0.001277);
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

        QHBoxLayout* view_layout = new QHBoxLayout();
        QWidget* view_widget = new QWidget();
        view_widget->setLayout(view_layout);
        opengl_viewer = new QOpenGL_LFViewer(this, microlens_view->getFinishedImage());
        view_layout->addWidget(opengl_viewer);
        opengl_viewer->update();

        QWidget* view_options_widget = new QWidget();
        QGridLayout* view_options_layout = new QGridLayout();
        view_options_widget->setLayout(view_options_layout);
        QDoubleSpinBox *overlap = new QDoubleSpinBox();
        overlap->setDecimals(7);
        overlap->setValue(14.29);
        connect( overlap, SIGNAL(valueChanged(double)), opengl_viewer, SLOT(setOverlap(double)) );
        view_options_layout->addWidget(new QLabel("Overlap:"),0,1);
        view_options_layout->addWidget(overlap,0,2);
        image_tab->addTab(view_widget,"View");
    }

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

void MainWindow::addTextTab(QWidget* widget, QString string){
    tabWidget->addTab(widget, string);
}
