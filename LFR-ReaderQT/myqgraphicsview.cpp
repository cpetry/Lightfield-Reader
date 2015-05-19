#include "myqgraphicsview.h"

//Qt includes
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QFileDialog>

/**
* Sets up the subclassed QGraphicsView
*/
MyGraphicsView::MyGraphicsView(QWidget* parent, QImage image) : QGraphicsView(parent) {

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //Set-up the scene
    Scene = new QGraphicsScene(this);
    setScene(Scene);

    this->image = image;
    setPixmap(QPixmap::fromImage(this->image));

    //Set-up the view
    setSceneRect(0, 0, this->image.width(), this->image.height());

    //Use ScrollHand Drag Mode to enable Panning
    setDragMode(ScrollHandDrag);
}

void MyGraphicsView::setPixmap(QPixmap pixmap){
    Scene->clear();
    Scene->addPixmap(pixmap);
    this->pixmap = pixmap;
}

void MyGraphicsView::demosaic(int type){
    QImage new_image(image.width(), image.height(), image.format());
    // mosaic: bayer
    // Gr R
    // Gb B
    if (type == 0){ // none
        for (int r=0; r < image.height(); r+=2){
            QRgb *new_scL1 = reinterpret_cast< QRgb *>( new_image.scanLine( r ) );
            QRgb *new_scL2 = reinterpret_cast< QRgb *>( new_image.scanLine( r+1 ) );
            QRgb *scL1 = reinterpret_cast< QRgb *>( image.scanLine( r ) );
            QRgb *scL2 = reinterpret_cast< QRgb *>( image.scanLine( r+1 ) );
            for (int c=0; c < image.width(); c+=2){
                //scL1[c]   = qRgb(0,             qRed(scL1[c]),0);       // Gr
                //scL1[c+1] = qRgb(qRed(scL1[c+1]),0,0);                  // R
                //scL1[c] = qRgb(0,qRed(scL1[c]),0);           // Gr
                new_scL1[c+1] = qRgb(qRed(scL1[c+1]),  0,0);   // R
                new_scL1[c]   = qRgb(0, qRed(scL1[c]),0);       // G
                new_scL2[c]   = qRgb(0,0, qRed(scL2[c]));        // B
                new_scL2[c+1] = qRgb(0,qRed(scL2[c+1]),0);       // Gb

            }
        }
    }

    // TODO
    // demosaic that sucker!
    if (type == 1){ // linear
        for (int r=0; r < image.height(); r+=2){
            QRgb *new_scL1 = reinterpret_cast< QRgb *>( new_image.scanLine( r ) );
            QRgb *new_scL2 = reinterpret_cast< QRgb *>( new_image.scanLine( r+1 ) );
            QRgb *scL1 = reinterpret_cast< QRgb *>( image.scanLine( r ) );
            QRgb *scL2 = reinterpret_cast< QRgb *>( image.scanLine( r+1 ) );
            for (int c=0; c < image.width(); c+=2){
                // Gr R
                // Gb B
                //int r = /*qGreen(scL1[c]) +*/ qRed(scL1[c+1]);// * 1.535;
                //int r = qRed(scL1[c]) + qRed(scL1[c+1]);// * 1.535;
                int r = qRed(scL1[c+1]) * 1.535;
                //int g = qRed(scL1[c]) + qRed(scL2[c]);
                int g = qRed(scL1[c]);
                //int b = qRed(scL2[c]) + qRed(scL2[c+1]);// * 1.222;
                int b = qRed(scL2[c]) * 1.222;
                //int b = qRed(scL2[c+1]) * 1.222;

                new_scL1[c]   = qRgb(r,g,b);
                new_scL1[c+1] = qRgb(r,g,b);
                new_scL2[c]   = qRgb(r,g,b);
                new_scL2[c+1] = qRgb(r,g,b);
            }
        }
    }

    QPixmap p = QPixmap::fromImage(new_image);
    this->setPixmap(p);
}

void MyGraphicsView::savePixmap(){
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");
    if (filename != ""){
        this->pixmap.save(filename);
    }
}

/**
  * Zoom the view in and out.
  */
void MyGraphicsView::wheelEvent(QWheelEvent* event) {

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Scale the view / do the zoom
    double scaleFactor = 1.15;
    if(event->delta() > 0) {
        // Zoom in
        scale(scaleFactor, scaleFactor);
    } else {
        // Zooming out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }

    // Don't call superclass handler here
    // as wheel is normally used for moving scrollbars
}
