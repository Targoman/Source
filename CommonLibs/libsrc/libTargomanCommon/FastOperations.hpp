/*************************************************************************
 * Copyright © 2012-2014, Targoman.com
 *
 * Published under the terms of TCRL(Targoman Community Research License)
 * You can find a copy of the license file with distributed source or
 * download it from http://targoman.com/License.txt
 *
 *************************************************************************/
/**
 @author S. Mohammad M. Ziabary <smm@ziabary.com>
 */

#ifndef TARGOMAN_COMMON_FASTOPERATIONS_HPP
#define TARGOMAN_COMMON_FASTOPERATIONS_HPP

#include "libTargomanCommon/Types.h"

namespace Targoman {
namespace Common {

const quint32 INT32_SIGN_BIT  = 0x80000000;

inline bool isNegativFloat(float _num){
    return ((FloatEncoded_t*)&_num)->AsUInt32 & INT32_SIGN_BIT;
}

inline bool isPositiveFloat(float _num){
    return ! isNegativFloat(_num);
}

inline quint32 rotl32 ( quint32 x, qint8 r )
{
  return (x << r) | (x >> (32 - r));
}

inline quint64 rotl64 ( quint32 x, qint8 r )
{
  return (x << r) | (x >> (64 - r));
}

inline std::string& fastTrimStdString(std::string& _str){
    const char* StrPtr = _str.c_str();
    char* EndStr = (char*)StrPtr + _str.size();

    while(*StrPtr){
        if(*StrPtr == ' ' ||
           *StrPtr == '\t' ||
           *StrPtr == '\r' ||
           *StrPtr == '\n')
            StrPtr++;
        else
            break;
    }
    while(*EndStr){
        if(*StrPtr == ' ' ||
           *StrPtr == '\t' ||
           *StrPtr == '\r' ||
           *StrPtr == '\n')
            EndStr--;
        else{
            *(EndStr+1) = '\0';
            break;
        }
    }
    return _str = StrPtr;
}

#define IS_WHITE_SPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_VALID_DIGIT(c) ((c) >= '0' && (c) <= '9')

inline float fastASCII2Float (const char *pFloatString, size_t& _lastPos)
{
    qint64 IntValue, Scale = 1;
    qint16 Sign;
    const char*  StartOfString = pFloatString;

    Sign = 1;
    if (*pFloatString == '-') {
        Sign = -1;
        ++pFloatString;
    } else if (*pFloatString == '+') {
        ++pFloatString;
    }

    // Get digits before decimal point or exponent, if any.
    for (IntValue = 0; IS_VALID_DIGIT(*pFloatString); ++pFloatString) {
        IntValue = IntValue * 10 + (*pFloatString - '0');
    }

    // Get digits after decimal point, if any.
    if (*pFloatString == '.') {
        pFloatString += 1;
        while (IS_VALID_DIGIT(*pFloatString)) {
            if (Scale < 10000000L){
                IntValue = IntValue * 10 + (*pFloatString - '0');
                Scale*=10;
            }
            ++pFloatString;
        }
    }

    _lastPos = pFloatString - StartOfString;
    // Return signed and scaled floating point result.
    return ((float)(Sign * IntValue))/(float)Scale;
}

inline const char* fastSkip2NonSpace(const char*& _str){
    while(*_str)
        if(*_str == ' ' || *_str == '\t' || *_str == '\r' || *_str == '\n')
            _str++;
        else
            return _str;
    return _str;
}

inline const char* fastSkip2Space(const char*& _str){
    while(*_str)
        if(*_str == ' ' || *_str == '\t' || *_str == '\r' || *_str == '\n')
            return _str;
        else
            _str++;
    return _str;
}

}
}


#endif // TARGOMAN_COMMON_FASTOPERATIONS_HPP