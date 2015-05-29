#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QWidget* parent, QImage image);
    void setRawPixmap(QPixmap pixmap);
    void setViewPixmap();
    void demosaic(int type);
    void savePixmap();
    void showLenslet();
    QImage getFinishedImage();

public slots:
    void tabSelected(int tab_number);
    void setLensletX(double x){ lenslet_center.setX(x); showLenslet();}
    void setLensletY(double y){ lenslet_center.setY(y); showLenslet();}
    void setLensletWidth(double w){ lenslet_size.setWidth(w); showLenslet();}
    void setLensletHeight(double h){ lenslet_size.setHeight(h); showLenslet();}
    void setLensletRotation(double r){ lenslet_rotation = r; showLenslet();}
    void setOverlap(double o){ overlap = o; setViewPixmap(); }

    void buttonGrayClicked(){ demosaic(0); }
    void buttonBayerClicked(){ demosaic(1); }
    void buttonDemosaicClicked(){ demosaic(2); }
    void buttonColorCorrectClicked(){ demosaic(3); }
    void toggleWhiteBalance(bool v){ enable_whiteBalance = v; }
    void toggleCCM(bool v){ enable_CCM = v; }
    void toggleGamma(bool v){ enable_Gamma = v; }
    void buttonSaveClicked(){ savePixmap(); }

protected:
    //Take over the interaction
    virtual void wheelEvent(QWheelEvent* event);

private:
    QGraphicsScene* Scene;
    QImage image, finished_image;
    QPixmap rawpixmap;
    QPointF lenslet_center;
    QSizeF lenslet_size;
    double lenslet_rotation;
    double overlap = 14.29;
    bool enable_whiteBalance = true, enable_CCM = true, enable_Gamma = true;

};

#endif // MYGRAPHICSVIEW_H
