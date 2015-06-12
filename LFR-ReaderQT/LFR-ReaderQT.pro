#-------------------------------------------------
#
# Project created by QtCreator 2015-04-16T13:49:43
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LFR-ReaderQT
TEMPLATE = app


SOURCES += main.cpp \
        mainwindow.cpp \
    lfp_reader.cpp \
    myqgraphicsview.cpp \
    qopengl_lfviewer.cpp

HEADERS  += mainwindow.h \
    lfp_reader.h \
    myqgraphicsview.h \
    qopengl_lfviewer.h

FORMS    += mainwindow.ui

DISTFILES += \
    lightfield_raw.fsh \
    lightfield_raw.vert \
    uvlightfield_focus.fsh \
    uvlightfield_focus.vert

## OpenCV settings for Windows and OpenCV 2.4.2
win32 {
    # MSVCRT link option (static or dynamic, it must be the same with your Qt SDK link option)
    #MSVCRT_LINK_FLAG_DEBUG = "/MDd"
    #MSVCRT_LINK_FLAG_RELEASE = "/MD"

    message("* Using settings for Windows.")
    INCLUDEPATH += $$(OPENCV_DIR)/include \
                   $$(OPENCV_DIR)/include/opencv \
                   $$(OPENCV_DIR)/include/opencv2

    LIBS += -L$$(OPENCV_DIR)/x64/vc12/lib \
            -L$$(OPENCV_DIR)/x64/vc12/bin

    CONFIG(debug, debug | release) {
        LIBS += -lopencv_calib3d2411d \
            -lopencv_contrib2411d \
            -lopencv_core2411d \
            -lopencv_features2d2411d \
            -lopencv_flann2411d \
            -lopencv_gpu2411d \
            -lopencv_highgui2411d \
            -lopencv_imgproc2411d \
            -lopencv_legacy2411d \
            -lopencv_ml2411d \
            -lopencv_nonfree2411d \
            -lopencv_objdetect2411d \
            -lopencv_photo2411d \
            -lopencv_stitching2411d \
            -lopencv_ts2411d \
            -lopencv_video2411d \
            -lopencv_videostab2411d
    }

    CONFIG(release, debug | release) {
        LIBS += -lopencv_calib3d2411 \
            -lopencv_contrib2411 \
            -lopencv_core2411 \
            -lopencv_features2d2411 \
            -lopencv_flann2411 \
            -lopencv_gpu2411 \
            -lopencv_highgui2411 \
            -lopencv_imgproc2411 \
            -lopencv_legacy2411 \
            -lopencv_ml2411 \
            -lopencv_nonfree2411 \
            -lopencv_objdetect2411 \
            -lopencv_photo2411 \
            -lopencv_stitching2411 \
            -lopencv_ts2411 \
            -lopencv_video2411 \
            -lopencv_videostab2411
    }
}
