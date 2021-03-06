#include "lfp_raw_view.h"

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
lfp_raw_view::lfp_raw_view(QWidget* parent, QImage image, LFP_Reader::lf_meta meta_infos) : QGraphicsView(parent) {
    this->meta_infos = meta_infos;
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //Set-up the scene
    Scene = new QGraphicsScene(this);
    setScene(Scene);

    this->image = image;
    setRawPixmap(QPixmap::fromImage(this->image));

    //Set-up the view
    setSceneRect(0, 0, this->image.width(), this->image.height());

    lenslet_center = QPointF(0,0);
    lenslet_size = QSize(10,10);
    lenslet_rotation = meta_infos.mla_rotation;

    //Use ScrollHand Drag Mode to enable Panning
    setDragMode(ScrollHandDrag);
}

void lfp_raw_view::setRawPixmap(QPixmap pixmap){
    Scene->clear();
    if (!pixmap.isNull())
        this->rawpixmap = pixmap;
    Scene->addPixmap(this->rawpixmap);
    //gi->setTransformationMode(Qt::SmoothTransformation);
}

void lfp_raw_view::showLenslet(){
    Scene->clear();
    Scene->addPixmap(rawpixmap);

    QColor c = QColor(Qt::red);
    QPen redpen = QPen(c);

    QPointF topleft = QPointF(rawpixmap.width()/2.0f+lenslet_center.x() - lenslet_size.width()/2.0f,
                              rawpixmap.height()/2.0f+lenslet_center.y() - lenslet_size.height()/2.0f);
    QPointF topright = QPointF(rawpixmap.width()/2.0f+lenslet_center.x() + lenslet_size.width()/2.0f,
                               rawpixmap.height()/2.0f+lenslet_center.y() - lenslet_size.height()/2.0f);
    QPointF bottomleft = QPointF(rawpixmap.width()/2.0f+lenslet_center.x() - lenslet_size.width()/2.0f,
                                 rawpixmap.height()/2.0f+lenslet_center.y() + lenslet_size.height()/2.0f);
    QPointF bottomright = QPointF(rawpixmap.width()/2.0f+lenslet_center.x() + lenslet_size.width()/2.0f,
                                  rawpixmap.height()/2.0f+lenslet_center.y() + lenslet_size.height()/2.0f);

    float HoriRotDiff = tan(lenslet_rotation) * rawpixmap.width()/2.0f;
    float VertRotDiff = tan(lenslet_rotation) * rawpixmap.height()/2.0f;

    Scene->addEllipse(topleft.x(), topleft.y(), lenslet_size.width(), lenslet_size.height(), redpen);
    Scene->addLine(0, topleft.y() - HoriRotDiff, this->rawpixmap.width(), topright.y() + HoriRotDiff, redpen);
    Scene->addLine(0, bottomleft.y() - HoriRotDiff, this->rawpixmap.width(), bottomright.y() + HoriRotDiff, redpen);
    Scene->addLine(topleft.x() + VertRotDiff, 0, bottomleft.x() - VertRotDiff, this->rawpixmap.height() , redpen);
    Scene->addLine(topright.x() + VertRotDiff, 0, bottomright.x() - VertRotDiff, this->rawpixmap.height() , redpen);
}

void lfp_raw_view::setViewPixmap(){
    //QColor c = QColor(Qt::red);
    //QPen redpen = QPen(c);
    QSize image_size = QSize(this->rawpixmap.width() / lenslet_size.width(), this->rawpixmap.height() / lenslet_size.height());
    QImage raw = rawpixmap.toImage();
    QImage image = QImage(QSize(625,433), QImage::Format_RGB32);
    //QImage weight = QImage(QSize(625,433), QImage::Format_RGB32);

    float yPerLensletToX = -tan(lenslet_rotation) * lenslet_size.width();
    float xPerLensletToY = tan(lenslet_rotation) * lenslet_size.height();

    QPointF lensCenterInRaw = QPointF(rawpixmap.width()/2.0f+lenslet_center.x(), rawpixmap.height()/2.0f+lenslet_center.y());
    float yPerLensletRow = sqrt(pow(lenslet_size.width(),2) - pow(lenslet_size.width()/2.0f,2));

    for (int lens_y= -215; lens_y < 215; lens_y++){
        for (int lens_x= -310; lens_x < 310; lens_x++){
            float curLensX = lensCenterInRaw.x() + lens_x*lenslet_size.width() - lens_y*xPerLensletToY;
            float curLensY = lensCenterInRaw.y() + lens_y*yPerLensletRow       - lens_x*yPerLensletToX;
            if (abs(lens_y) % 2 == 1)
                curLensX -= 0.5f*lenslet_size.width();

            // calculate position of current lense center
            QPointF curLensCenter = QPointF(curLensX, curLensY);

            // DEBUG
            //Scene->addEllipse(curLensCenter.x()-lenslet_size.width()/2, curLensCenter.y()-lenslet_size.height()/2
            //                  ,lenslet_size.width(), lenslet_size.height(), redpen);

            if (curLensCenter.x() < 0 + lenslet_size.width() || curLensCenter.x() > raw.width()  - lenslet_size.width()
            ||  curLensCenter.y() < 0 + lenslet_size.height()|| curLensCenter.y() > raw.height() - lenslet_size.height())
                continue;

            // calculate image position in dependence of center lense
            float image_diffy = (lensCenterInRaw.y() - curLensCenter.y());
            float image_diffx = (lensCenterInRaw.x() - curLensCenter.x());
            int image_posy = int((lensCenterInRaw.y() - image_diffy + 0.5) * image.size().height()/raw.height());
            int image_posx = int((lensCenterInRaw.x() - image_diffx + 0.5) * image.size().width()/raw.width());

            QPointF pf = curLensCenter;
            QPoint  p  = QPoint(int(pf.x()), int(pf.y()));
            float fx = pf.x() - p.x();
            float fy = pf.y() - p.y();
            float fx1 = 1.0f - fx;
            float fy1 = 1.0f - fy;

            float w1 = fx1 * fy1;
            float w2 = fx  * fy1;
            float w3 = fx1 * fy;
            float w4 = fx  * fy;

            // Calculate the weighted sum of pixels (for each color channel)
            QRgb p1 = raw.pixel(p);
            QRgb p2 = raw.pixel(QPoint(p.x()+1,p.y()  ));
            QRgb p3 = raw.pixel(QPoint(p.x()  ,p.y()+1));
            QRgb p4 = raw.pixel(QPoint(p.x()+1,p.y()+1));
            int outr = qRed(p1)   * w1 + qRed(p2)   * w2 + qRed(p3)   * w3 + qRed(p4)   * w4;
            int outg = qGreen(p1) * w1 + qGreen(p2) * w2 + qGreen(p3) * w3 + qGreen(p4) * w4;
            int outb = qBlue(p1)  * w1 + qBlue(p2)  * w2 + qBlue(p3)  * w3 + qBlue(p4)  * w4;

            image.setPixel(QPoint(image_posx, image_posy), qRgb(outr, outg, outb));
        }
    }

    Scene->clear();
    Scene->addPixmap(QPixmap::fromImage(image));

}

QImage lfp_raw_view::demosaic(int type){
    QImage raw_image = image.convertToFormat(QImage::Format_RGB32);
    QImage new_image(image.width(), image.height(), QImage::Format_RGB32);

    if(type == 0){ // none
        new_image = image.copy();
    }
    // mosaic: bayer
    // Gr R
    // B  Gb
    else if (type == 1){ // bayer
        for (int r=0; r < raw_image.height(); r+=2){
            QRgb *new_scL1 = reinterpret_cast< QRgb *>( new_image.scanLine( r ) );
            QRgb *new_scL2 = reinterpret_cast< QRgb *>( new_image.scanLine( r+1 ) );
            QRgb *scL1 = reinterpret_cast< QRgb *>( raw_image.scanLine( r ));
            QRgb *scL2 = reinterpret_cast< QRgb *>( raw_image.scanLine( r+1 ));
            for (int c=0; c < raw_image.width(); c+=2){
                //scL1[c]   = qRgb(0,             qRed(scL1[c]),0);       // Gr
                //scL1[c+1] = qRgb(qRed(scL1[c+1]),0,0);                  // R
                //scL1[c] = qRgb(0,qRed(scL1[c]),0);           // Gr
                new_scL1[c+1] = qRgb(qRed(scL1[c+1]) * meta_infos.r_bal ,  0,0);   // R
                new_scL1[c]   = qRgb(0, qRed(scL1[c]),0);       // G
                new_scL2[c]   = qRgb(0,0, qRed(scL2[c]) * meta_infos.b_bal);        // B
                new_scL2[c+1] = qRgb(0,qRed(scL2[c+1]),0);       // Gb

            }
        }
    }

    // TODO
    // demosaic that sucker!
    else if (type == 2 || type == 3){

        // bilinear
        for (int r=0; r < raw_image.height(); r+=2){
            QRgb *new_scL1 = reinterpret_cast< QRgb *>( new_image.scanLine( r ) );
            QRgb *new_scL2 = reinterpret_cast< QRgb *>( new_image.scanLine( r+1 ) );
            QRgb *scL1 = reinterpret_cast< QRgb *>( raw_image.scanLine( r ) );
            QRgb *scL2 = reinterpret_cast< QRgb *>( raw_image.scanLine( r+1 ) );
            for (int c=0; c < raw_image.width(); c+=2){
                QPoint b1 = QPoint(c,(r-1)>=0 ? r-1 : r);
                QPoint b2 = QPoint((c+2)<raw_image.width() ? (c+2):c,(r-1)>=0 ? r-1 : r);
                QPoint b3 = QPoint(c,r+1);
                QPoint b4 = QPoint(c+2<raw_image.width()?c+2:c,r+1);

                QPoint r1 = QPoint(c-1>=0?(c-1):(c+1),r);
                QPoint r2 = QPoint(c+1,r);
                QPoint r3 = QPoint(c-1>=0?(c-1):(c+1),(r+2)<raw_image.height() ? r+2 : r);
                QPoint r4 = QPoint(c+1,(r+2)<raw_image.height() ? r+2 : r);

                QPoint g1 = QPoint(c+1,(r-1)>=0 ? r-1 : r);
                QPoint g2 = QPoint(c,r);
                QPoint g3 = QPoint(c+2<raw_image.width()?c+2:c,r);
                QPoint g4 = QPoint(c-1>=0?(c-1):(c+1),r+1);
                QPoint g5 = QPoint(c+1,r+1);
                QPoint g6 = QPoint(c,(r+2)<raw_image.height() ? r+2 : r);

                /*int red  = qRed(scL1[c+1]);    //
                int r_l  = (c>1) ? qRed(scL1[c-1]) : red; // left
                int r_b  = (r+2<raw_image.height()) ? qRed(raw_image.pixel(c+1,r+2)) : red; // bottom
                int r_lb = (r+2<raw_image.height() && (c>1)) ? qRed(raw_image.pixel(c-1,r+2)) : red; // left-bottom
                int blue = qRed(scL2[c+1]);    //
                int b_t  = (r>1) ? qRed(raw_image.pixel(c,r-1)) : blue; // top
                int b_tr = (r-2>=0 && c+2<raw_image.width()) ? qRed(raw_image.pixel(c+2,r-1)) : blue; // left-bottom
                int b_r  = (c+2<raw_image.width()) ? qRed(raw_image.pixel(c+2,r+1)) : blue; // bottom

                int greenr = qRed(scL2[c+1]);    //
                int gr_t  = (r>1) ? qRed(raw_image.pixel(c,r-1)) : blue; // top
                int b_tr = (r-2>=0 && c+2<raw_image.width()) ? qRed(raw_image.pixel(c+2,r-1)) : blue; // left-bottom
                int b_r  = (c+2<raw_image.width()) ? qRed(raw_image.pixel(c+2,r+1)) : blue; // bottom*/
                // Gr
                new_scL1[c]   = qRgb((qRed(raw_image.pixel(r1)) + qRed(raw_image.pixel(r2))+0.5) / 2,
                                      qRed(raw_image.pixel(g2)),
                                     (qRed(raw_image.pixel(b1)) + qRed(raw_image.pixel(b3))+0.5) / 2);

                // R
                new_scL1[c+1] = qRgb(qRed(raw_image.pixel(r2)),
                                     (qRed(raw_image.pixel(g1)) + qRed(raw_image.pixel(g2)) +
                                      qRed(raw_image.pixel(g3)) + qRed(raw_image.pixel(g5)) +0.5) / 4,
                                     (qRed(raw_image.pixel(b1)) + qRed(raw_image.pixel(b2)) +
                                      qRed(raw_image.pixel(b3)) + qRed(raw_image.pixel(b4)) +0.5) / 4);

                // B
                new_scL2[c]   = qRgb((qRed(raw_image.pixel(r1)) + qRed(raw_image.pixel(r2)) +
                                      qRed(raw_image.pixel(r3)) + qRed(raw_image.pixel(r4)) +0.5) / 4,
                                     (qRed(raw_image.pixel(g2)) + qRed(raw_image.pixel(g4)) +
                                      qRed(raw_image.pixel(g5)) + qRed(raw_image.pixel(g6)) +0.5) / 4,
                                     qRed(raw_image.pixel(b3)));

                // Gb
                new_scL2[c+1] = qRgb((qRed(raw_image.pixel(r2)) + qRed(raw_image.pixel(r4))+0.5) / 2,
                                     qRed(raw_image.pixel(g5)),
                                    (qRed(raw_image.pixel(b3)) + qRed(raw_image.pixel(b4))+0.5) / 2);
            }
        }
    }

    // Color correct
    if (type == 3){ // linear
        QMatrix3x3 c(meta_infos.cc);
        QMatrix4x4 ccm = QMatrix4x4(c.data(), 3, 3);
        //ccm = ccm.inverted();
        float r_bal = meta_infos.r_bal;
        float b_bal = meta_infos.b_bal;
        float SatLvl_r = r_bal*ccm(0,0) + 1*ccm(0,1) + b_bal*ccm(0,2);
        float SatLvl_g = r_bal*ccm(1,0) + 1*ccm(1,1) + b_bal*ccm(1,2);
        float SatLvl_b = r_bal*ccm(2,0) + 1*ccm(2,1) + b_bal*ccm(2,2);
        float min_sat = std::fmin(std::fmin(SatLvl_r, SatLvl_g), SatLvl_b);

        for(int row=0; row< new_image.height(); row++){
            QRgb *scL = reinterpret_cast< QRgb *>( new_image.scanLine( row ) );
            for(int col=0; col < new_image.width(); col++){

                // not possible to get correct results without proper bayer algorithm.
                // so just trying to approximate that
                float r = qRed(scL[col])   / 255.0;
                float g = qGreen(scL[col]) / 255.0;
                float b = qBlue(scL[col])  / 255.0;

                // white balance
                if (enable_whiteBalance){
                    r *= r_bal;
                    b *= b_bal;
                }

                // color correction matrix
                float new_r = r;
                float new_g = g;
                float new_b = b;

                if (enable_CCM){
                    new_r = (r * ccm(0,0)) + (g * ccm(0,1)) + (b * ccm(0,2));
                    new_g = (r * ccm(1,0)) + (g * ccm(1,1)) + (b * ccm(1,2));
                    new_b = (r * ccm(2,0)) + (g * ccm(2,1)) + (b * ccm(2,2));
                    //new_r = fmax(0,fmin(new_r, min_sat)) / min_sat;
                    //new_g = fmax(0,fmin(new_g, min_sat)) / min_sat;
                    //new_b = fmax(0,fmin(new_b, min_sat)) / min_sat;
                }

                if (enable_Gamma){
                    float gamma = 0.416660f;
                    new_r = pow(new_r, gamma);
                    new_g = pow(new_g, gamma);
                    new_b = pow(new_b, gamma);
                }

                scL[col] = qRgb(std::min(int(new_r*255),255),std::min(int(new_g*255),255),std::min(int(new_b*255),255));
            }
        }
        finished_image = new_image;
    }
    QPixmap p = QPixmap::fromImage(new_image);
    this->setRawPixmap(p);
    return new_image;
}

QImage lfp_raw_view::getFinishedImage(){
    demosaic(3);
    return finished_image;
}

void lfp_raw_view::savePixmap(){
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");
    if (filename != ""){
        this->rawpixmap.save(filename);
    }

    saveMetaInfo(filename);
}

void lfp_raw_view::saveRaw(){
    QString filename = QFileDialog::getSaveFileName(0,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Images (*.png *.xpm *.jpg)");

    QVector<QRgb> colorTable;
    QImage retImg(image.width(),image.height(),QImage::Format_Indexed8);
    QVector<QRgb> table( 256 );
    for( int i = 0; i < 256; ++i )
    {
        table[i] = qRgb(i,i,i);
    }
    retImg.setColorTable(table);
    for(int i =0; i< image.width();i++)
    {
        for(int j=0; j< image.height();j++)
        {
            QRgb value = image.pixel(i,j);
            retImg.setPixel(i,j,qRed(value));
        }

    }

    if (filename != ""){
        retImg.save(filename);
    }

    saveMetaInfo(filename);
}

void lfp_raw_view::saveMetaInfo(QString filename){
    QString meta_string = LFP_Reader::createMetaInfoText(this->meta_infos);

    filename = filename.split(".")[0] + ".txt";
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << meta_string << endl;
    }
    file.close();
}

/**
  * Zoom the view in and out.
  */
void lfp_raw_view::wheelEvent(QWheelEvent* event) {

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
