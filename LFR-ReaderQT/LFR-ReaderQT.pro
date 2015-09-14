#-------------------------------------------------
#
# Project created by QtCreator 2015-04-16T13:49:43
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LFR-ReaderQT
TEMPLATE = app

isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    DEPLOY_COMMAND = windeployqt
}
macx {
    DEPLOY_COMMAND = macdeployqt
}

SOURCES += main.cpp \
        mainwindow.cpp \
    lfp_reader.cpp \
    myqgraphicsview.cpp \
    qopengl_lfviewer.cpp \
    reconstruction3d.cpp \
    imagedepth.cpp \
    lfp_raw_view.cpp \
    calibration.cpp \
    depthfromfocus.cpp \
    depthcostaware.cpp \
    depthredefinedisparity.cpp \
    depthstereolike.cpp \
    depthstereo.cpp \
    depthfromepipolarimages.cpp

HEADERS  += mainwindow.h \
    lfp_reader.h \
    myqgraphicsview.h \
    qopengl_lfviewer.h \
    reconstruction3d.h \
    imagedepth.h \
    lfp_raw_view.h \
    calibration.h \
    depthfromfocus.h \
    depthcostaware.h \
    depthredefinedisparity.h \
    depthstereolike.h \
    depthstereo.h \
    depthfromepipolarimages.h

FORMS    += mainwindow.ui

DISTFILES +=

RESOURCES += \
    shaders.qrc

## OpenCV settings for Windows and OpenCV 2.4.2
win32 {
    # MSVCRT link option (static or dynamic, it must be the same with your Qt SDK link option)
    #MSVCRT_LINK_FLAG_DEBUG = "/MDd"
    #MSVCRT_LINK_FLAG_RELEASE = "/MD"

    message("* Using settings for Windows.")
    INCLUDEPATH += $$(OPENCV_DIR)/include \
                   $$(OPENCV_DIR)/include/opencv \
                   $$(OPENCV_DIR)/include/opencv2 \
                   $$(PCL_ROOT)/include/pcl-1.7 \
                   $$(PCL_ROOT)/3rdParty/Boost/include/boost-1_57 \
                   $$(PCL_ROOT)/3rdParty/Eigen/eigen3 \
                   $$(PCL_ROOT)/3rdParty/VTK/include/vtk-6.2 \
                   $$(PCL_ROOT)/3rdParty/FLANN/include \
                   $$(AMDAPPSDKROOT)/include

    LIBS += -L$$(OPENCV_DIR)/x64/vc12/lib \
            -L$$(OPENCV_DIR)/x64/vc12/bin \
            -L$$(PCL_ROOT)/lib \
            -L$$(PCL_ROOT)/bin \
            -L$$(PCL_ROOT)/3rdParty/Boost/lib \
            -L$$(PCL_ROOT)/3rdParty/VTK/lib \
            -L$$(PCL_ROOT)/3rdParty/VTK/bin \
            -L$$(PCL_ROOT)/3rdParty/FLANN/lib \
            -L$$(PCL_ROOT)/3rdParty/FLANN/bin \
            -L./dependencies

    CONFIG(debug, debug | release) {
        DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/debug/$${TARGET}$${TARGET_CUSTOM_EXT}))
        LIBS += -lopencv_calib3d2411d \
            -lopencv_contrib2411d \
            -lopencv_core2411d \
            -lopencv_features2d2411d \
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
            -lopencv_videostab2411d \
            -lpcl_search_debug \
            -lpcl_common_debug \
            -lpcl_features_debug \
            -lpcl_io_debug \
            -lpcl_surface_debug \
            -lpcl_visualization_debug \
            -lvtkCommonDataModel-6.2 \
            -lvtkCommonCore-6.2 \
            -luser32
    }

    CONFIG(release, debug | release) {
        DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}))
        LIBS += -lopencv_calib3d2411 \
            -lopencv_contrib2411 \
            -lopencv_core2411 \
            -lopencv_features2d2411 \
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
            -lopencv_videostab2411 \
            -lpcl_search_release \
            -lpcl_common_release \
            -lpcl_features_release \
            -lpcl_io_release \
            -lpcl_surface_release \
            -lpcl_visualization_release \
            -lvtkCommonDataModel-6.2 \
            -lvtkCommonCore-6.2 \
            -luser32
    }
}
