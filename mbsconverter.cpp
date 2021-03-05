#include <string>
#include <clocale>
#include "mbsconverter.h"

namespace html{
    using std::wstring;
    using std::string;

    void mbstowcs(wstring& wcs, const string& s){
        const char* oldlocale = std::setlocale(LC_ALL, NULL);
        std::setlocale(LC_ALL, "zh_CN.UTF-8");
        size_t targetlen = std::mbstowcs(NULL, s.c_str(), 0);
        wchar_t* wcsbuf = new wchar_t[targetlen+1];
        std::mbstowcs(wcsbuf, s.c_str(), targetlen);
        wcsbuf[targetlen] = L'\0';
        wcs = wcsbuf;
        delete[] wcsbuf;
        std::setlocale(LC_ALL, oldlocale);
    }

    void wcstombs(string& s, const wstring& wcs){
        const char* oldlocale = std::setlocale(LC_ALL, NULL);
        std::setlocale(LC_ALL, "zh_CN.UTF-8");
        size_t targetlen = std::wcstombs(NULL, wcs.c_str(), 0);
        char* sbuf = new char[targetlen+1];
        std::wcstombs(sbuf, wcs.c_str(), targetlen);
        sbuf[targetlen] = '\0';
        s = sbuf;
        delete[] sbuf;
        std::setlocale(LC_ALL, oldlocale);
    } 
}