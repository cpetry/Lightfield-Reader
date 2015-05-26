#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lfp_reader.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionFile, &QAction::triggered, this, &MainWindow::chooseFile);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chooseFile(){
    QString file = QFileDialog::getOpenFileName(this, QString("Choose Lightfield File"), "../", QString("LightFields(*.LFP *.RAW)"));
    if (!file.isNull()){
        ui->statusBar->showMessage("Loading Lightfield...");
        if (file.endsWith("RAW", Qt::CaseInsensitive))
            LFP_Reader::read_RAWFile(this, file.toStdString());
        else if (file.endsWith("lfp", Qt::CaseInsensitive))
            LFP_Reader::read_lfp(this, file.toStdString());
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
    ui->tabWidget->addTab(complete_widget, TabString);
}

void MainWindow::setupView(QString header, QString sha1, int sec_length, QImage image, bool raw_image){
    QLabel* l_header = new QLabel("Header: " + header.toLatin1().toHex());
    QLabel* l_sha1 = new QLabel("Sha1: " + sha1);
    QLabel* l_infos = new QLabel("Bytes: " + QString::number(sec_length));
    QLabel* imagesize = new QLabel("Size: " + QString::number(image.width()) + "x" + QString::number(image.height()));

    view = new MyGraphicsView(NULL, image);
    QHBoxLayout* h_layout = new QHBoxLayout();
    h_layout->addWidget(view);

    if (raw_image){
        QPushButton *gray = new QPushButton("Gray");
        QPushButton *bayer = new QPushButton("Bayer");
        QPushButton *demosaic = new QPushButton("Demosaic");
        QPushButton *save = new QPushButton("Save");
        connect( gray, SIGNAL(clicked()), this, SLOT(buttonGrayClicked()) );
        connect( bayer, SIGNAL(clicked()), this, SLOT(buttonBayerClicked()) );
        connect( demosaic, SIGNAL(clicked()), this, SLOT(buttonDemosaicClicked()) );
        connect( save, SIGNAL(clicked()), this, SLOT(buttonSaveClicked()) );
        QVBoxLayout* button_layout = new QVBoxLayout();
        button_layout->addWidget(gray);
        button_layout->addWidget(bayer);
        button_layout->addWidget(demosaic);
        button_layout->addWidget(save);
        QWidget* button_widget = new QWidget();
        button_widget->setLayout(button_layout);
        h_layout->addWidget(button_widget);
    }

    QWidget* image_view_widget = new QWidget();
    image_view_widget->setLayout(h_layout);

    QWidget* complete_widget = new QWidget();
    QVBoxLayout* v_layout = new QVBoxLayout();
    v_layout->addWidget(l_header);
    v_layout->addWidget(l_sha1);
    v_layout->addWidget(l_infos);
    v_layout->addWidget(imagesize);
    v_layout->addWidget(image_view_widget);
    complete_widget->setLayout(v_layout);
    ui->tabWidget->addTab(complete_widget, "Image");
}

void MainWindow::addTextTab(QWidget* widget, QString string){
    ui->tabWidget->addTab(widget, string);
}

void MainWindow::buttonGrayClicked(){
    this->view->demosaic(0); // nearest neighbour
}

void MainWindow::buttonBayerClicked(){
    this->view->demosaic(1); // nearest neighbour
}

void MainWindow::buttonDemosaicClicked(){
    this->view->demosaic(2); // nearest neighbour
}

void MainWindow::buttonSaveClicked(){
    this->view->savePixmap();
}

