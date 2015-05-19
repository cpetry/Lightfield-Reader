#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>

class MyGraphicsView : public QGraphicsView
{
public:
    MyGraphicsView(QWidget* parent, QImage image);
    void setPixmap(QPixmap pixmap);
    void demosaic(int type);
    void savePixmap();

protected:
    //Take over the interaction
    virtual void wheelEvent(QWheelEvent* event);

private:
    QGraphicsScene* Scene;
    QImage image;
    QPixmap pixmap;

};

#endif // MYGRAPHICSVIEW_H
