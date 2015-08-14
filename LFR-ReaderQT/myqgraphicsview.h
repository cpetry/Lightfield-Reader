#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>

#include "lfp_reader.h"

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QWidget* parent);
    void setPixmap(QPixmap pixmap);

protected:
    //Take over the interaction
    virtual void wheelEvent(QWheelEvent* event);

private:
    QGraphicsScene* Scene;
    QImage image;
    QPixmap rawpixmap;

};

#endif // MYGRAPHICSVIEW_H
