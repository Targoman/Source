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

#ifndef TARGOMAN_NLPLIBS_PRIVATE_CLSLANGUAGEMODEL_P_H
#define TARGOMAN_NLPLIBS_PRIVATE_CLSLANGUAGEMODEL_P_H

#include "libTargomanCommon/Macros.h"
#include "../clsLanguageModel.h"
#include "clsVocab.hpp"
#include "clsBaseModel.hpp"

namespace Targoman {
namespace NLPLibs {
namespace Private {

class clsLanguageModelPrivate
{
public:
    clsLanguageModelPrivate();
    bool isBinary(const QString& _file);

public:
    quint8               Order;
    clsBaseModel*        Model;
};


}
}
}
#endif // TARGOMAN_NLPLIBS_PRIVATE_CLSLANGUAGEMODEL_P_H
