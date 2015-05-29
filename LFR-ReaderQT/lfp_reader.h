#pragma once

#include <string>

#include <QImage>
#include <QTextBrowser>
#include <QPushButton>
#include "aspectratiopixmaplabel.h"

class MainWindow;

class LFP_Reader
{

public:
    LFP_Reader();

    struct lf_meta{
        lf_meta(int bits, int width, int height){
            this->bits = bits;
            this->width = width;
            this->height = height;
        }
        int bits;
        int width;
        int height;
    };

    lf_meta meta_infos = lf_meta(10,20,20);

    bool read_lfp(MainWindow *main, std::string file);
    bool read_RAWFile(MainWindow* main, std::string file);
    void parseLFMetaInfo(QString meta_info);
    std::vector<char> readBytes(std::basic_ifstream<unsigned char> &input, int nmb_bytes = 0);
    std::string  readText(std::basic_ifstream<unsigned char> &input, int nmb_bytes = 0);

private:

    enum HEADER_TYPE{
        NONE, TYPE_LFP, TYPE_TEXT, TYPE_PICTURE, TYPE_RAWPICTURE, TYPE_IGNORE
    };

    bool readSection(MainWindow *main, std::basic_ifstream<unsigned char> &input, HEADER_TYPE section_type);
    QImage readRawPixelData(MainWindow *main, std::basic_ifstream<unsigned char> &input, int section_length);

};
