#include "myqgraphicsview.h"

#include <stdint.h>
#include <cmath>
//Qt includes
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QFileDialog>
#include <QMatrix3x3>
#include <QMatrix4x4>

/**
* Sets up the subclassed QGraphicsView
*/
MyGraphicsView::MyGraphicsView(QWidget* parent) : QGraphicsView(parent) {
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //Set-up the scene
    Scene = new QGraphicsScene(this);
    setScene(Scene);

    //Set-up the view
    setSceneRect(0, 0, this->image.width(), this->image.height());

    //Use ScrollHand Drag Mode to enable Panning
    setDragMode(ScrollHandDrag);
}

void MyGraphicsView::setPixmap(QPixmap pixmap){
    //Set-up the view
    setSceneRect(0, 0, pixmap.width(), pixmap.height());

    Scene->clear();
    if (!pixmap.isNull())
        this->rawpixmap = pixmap;
    Scene->addPixmap(this->rawpixmap);
    //gi->setTransformationMode(Qt::SmoothTransformation);
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
