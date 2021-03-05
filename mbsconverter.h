
#ifndef __MBSCONVERTER__
#define __MBSCONVERTER__
#include <string>
namespace html{
    void mbstowcs(std::wstring& wcs, const std::string& s);
    void wcstombs(std::string& s, const std::wstring& wcs);
}
#endif