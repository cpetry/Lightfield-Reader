#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lfp_reader.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //LFP_Reader::read_lfp(ui, "../IMG_0004.lfr");
    //LFP_Reader::read_lfp(ui, "../lytro-illum-raw-sample-image-3-sauces.lfr");
    LFP_Reader::read_lfp(this, "../frenchguy.lfp");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupView(QImage image){
    view = new MyGraphicsView(NULL, image);
    QHBoxLayout* h_layout = new QHBoxLayout();
    h_layout->addWidget(view);

    QPushButton *bayer = new QPushButton("Bayer");
    QPushButton *demosaic = new QPushButton("Demosaic");
    QPushButton *save = new QPushButton("Save");
    connect( bayer, SIGNAL(clicked()), this, SLOT(buttonBayerClicked()) );
    connect( demosaic, SIGNAL(clicked()), this, SLOT(buttonDemosaicClicked()) );
    connect( save, SIGNAL(clicked()), this, SLOT(buttonSaveClicked()) );
    QVBoxLayout* button_layout = new QVBoxLayout();
    button_layout->addWidget(bayer);
    button_layout->addWidget(demosaic);
    button_layout->addWidget(save);
    QWidget* button_widget = new QWidget();
    button_widget->setLayout(button_layout);
    h_layout->addWidget(button_widget);

    QWidget* image_view_widget = new QWidget();
    image_view_widget->setLayout(h_layout);
    ui->tabWidget->addTab(image_view_widget, "Image");
}

void MainWindow::addTextTab(QWidget* widget, QString string){
    ui->tabWidget->addTab(widget, string);
}

void MainWindow::buttonDemosaicClicked(){
    this->view->demosaic(1); // nearest neighbour
}

void MainWindow::buttonBayerClicked(){
    this->view->demosaic(0); // nearest neighbour
}

void MainWindow::buttonSaveClicked(){
    this->view->savePixmap();
}

