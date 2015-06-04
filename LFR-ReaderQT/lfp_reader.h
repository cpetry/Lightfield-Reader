#pragma once

#include <string>

#include <QImage>
#include <QTextBrowser>
#include <QPushButton>

// helper
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include <streambuf>

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

class MainWindow;

class LFP_Reader
{

public:
    LFP_Reader(){};

    struct lf_meta{
        lf_meta(){}
        int bits;
        int width;
        int height;
        double modulationExposureBias;
        float cc[9];
        float r_bal;
        float b_bal;
        double mla_rotation;
        double mla_scale_x;
        double mla_scale_y;
        double mla_lensPitch;
        double mla_pixelPitch;
        double mla_centerOffset_x;
        double mla_centerOffset_y;
    };

    lf_meta meta_infos;

    bool read_lfp(MainWindow *main, std::string file, bool save_raw_to_file = false, std::string raw_file_name = "");
    bool read_RAWFile(MainWindow* main, std::string file);
    void parseLFMetaInfo(QString meta_info);
    std::vector<char> readBytes(std::basic_ifstream<unsigned char> &input, int nmb_bytes = 0);
    std::string readText(std::basic_ifstream<unsigned char> &input, int nmb_bytes = 0);

private:

    enum HEADER_TYPE{
        NONE, TYPE_LFP, TYPE_TEXT, TYPE_PICTURE, TYPE_RAWPICTURE, TYPE_IGNORE
    };

    bool readSection(MainWindow *main, std::basic_ifstream<unsigned char> &input,
                     HEADER_TYPE section_type, std::string raw_file_name = 0);
    QImage readRawPixelData(std::basic_ifstream<unsigned char> &input, int section_length);



    // helper functions
    struct membuf : std::streambuf
    {
        membuf(std::vector<char> &v) {
            this->setg(v.data(), v.data(), v.data() + v.size());
        }
    };

    // trim from start
    static inline std::string &ltrim(std::string &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
            return s;
    }

    // trim from end
    static inline std::string &rtrim(std::string &s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
            return s;
    }

    // trim from both ends
    static inline std::string &trim(std::string &s) {
            return ltrim(rtrim(s));
    }

    static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    static std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }

    static std::string getValueOf(std::string searchstr, std::string heystack, int from){
        size_t pos = heystack.find(":", heystack.find(searchstr, from));
        int str_size = 0;
        if (heystack.find("[", pos) < heystack.find(",", pos)){
            pos = heystack.find("[", pos)+1;
            str_size = int(heystack.find("]", pos) - pos);
        }
        else{
            str_size = int(std::min(heystack.find(",", pos), heystack.find("}", pos)) - pos);
        }
        return trim(heystack.substr(pos+1, str_size-1));
    }
};
