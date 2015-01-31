/*************************************************************************
 * Copyright © 2012-2015, Targoman.com
 *
 * Published under the terms of TCRL(Targoman Community Research License)
 * You can find a copy of the license file with distributed source or
 * download it from http://targoman.com/License.txt
 *
 *************************************************************************/
/**
 @author S. Mohammad M. Ziabary <smm@ziabary.com>
 @author Behrooz Vedadian <vedadian@gmail.com>
 */

#ifndef TARGOMAN_NLPLIBS_LM_DEFINITIONS_H
#define TARGOMAN_NLPLIBS_LM_DEFINITIONS_H

#include "libTargomanCommon/exTargomanBase.h"
#include "libTargomanCommon/Macros.h"
#include "libTargomanCommon/Types.h"

namespace Targoman {
namespace NLPLibs {

TARGOMAN_ADD_EXCEPTION_HANDLER(exLanguageModel, Targoman::Common::exTargomanBase);

TARGOMAN_DEFINE_ENHANCED_ENUM_BEGIN(enuMemoryModel)
    Auto,           //Auto-detected based on LM file
    Plain,          //Plain ARPA with no binarization
    Probing,        //probing hash tables
    RestProbing,    //probing hash tables with rest costs
TARGOMAN_DEFINE_ENHANCED_ENUM_STRINGS
    "Auto",
    "Plain",
    "Probing",
    "RestProbing",
TARGOMAN_DEFINE_ENHANCED_ENUM_END

//Definitions for special string in ARPA file
extern const char* LM_UNKNOWN_WORD;
extern const char* LM_BEGIN_SENTENCE;
extern const char* LM_END_SENTENCE;

//This constant is used to quickly check whether input file is valid ARPA plain text file or not.
//Total lenght of an ARPA must be less than indicated number
const quint16   LM_MAX_VALID_ARPA_LINE = 1000;
//Maximum order supported by LM
const quint8    LM_MAX_ORDER = 6;

struct stuLMConfigs
{
    struct{
        Targoman::Common::LogP_t Prob;
        Targoman::Common::LogP_t Backoff;
    }UnknownWordDefault;

    stuLMConfigs(){
        this->UnknownWordDefault.Prob = 0;
        this->UnknownWordDefault.Backoff = 0;
    }
};

struct stuLMResult{
    Targoman::Common::LogP_t  Prob;
    quint8  NGram;
    inline stuLMResult(Targoman::Common::LogP_t _prob, quint8 _ngram){
        this->NGram = _ngram;
        this->Prob = _prob;
    }
};

}
}

#endif // TARGOMAN_NLPLIBS_LM_DEFINITIONS_H