#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>

#include "lfp_reader.h"

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QWidget* parent, QImage image, LFP_Reader::lf_meta meta_infos);
    void setRawPixmap(QPixmap pixmap);
    void setViewPixmap();
    QImage demosaic(int type);
    void showLenslet();
    QImage getFinishedImage();

public slots:
    void setLensletX(double x){ lenslet_center.setX(x); showLenslet();}
    void setLensletY(double y){ lenslet_center.setY(y); showLenslet();}
    void setLensletWidth(double w){ lenslet_size.setWidth(w); showLenslet();}
    void setLensletHeight(double h){ lenslet_size.setHeight(h); showLenslet();}
    void setLensletRotation(double r){ lenslet_rotation = r; showLenslet();}

    void buttonGrayClicked(){ demosaic(0); }
    void buttonBayerClicked(){ demosaic(1); }
    void buttonDemosaicClicked(){ demosaic(2); }
    void buttonColorCorrectClicked(){ demosaic(3); }
    void toggleWhiteBalance(bool v){ enable_whiteBalance = v; }
    void toggleCCM(bool v){ enable_CCM = v; }
    void toggleGamma(bool v){ enable_Gamma = v; }
    void savePixmap();
    void saveRaw();
    void saveMetaInfo(QString filename);

protected:
    //Take over the interaction
    virtual void wheelEvent(QWheelEvent* event);

private:
    QGraphicsScene* Scene;
    QImage image, finished_image;
    QPixmap rawpixmap;
    QPointF lenslet_center = QPointF(0,0);
    QSizeF lenslet_size = QSize(10,10);
    double lenslet_rotation;
    bool enable_whiteBalance = true, enable_CCM = true, enable_Gamma = true;
    LFP_Reader::lf_meta meta_infos;

};

#endif // MYGRAPHICSVIEW_H
