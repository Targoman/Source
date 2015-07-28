/******************************************************************************
 * Targoman: A robust Statistical Machine Translation framework               *
 *                                                                            *
 * Copyright 2014-2015 by ITRC <http://itrc.ac.ir>                            *
 *                                                                            *
 * This file is part of Targoman.                                             *
 *                                                                            *
 * Targoman is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Targoman is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU Lesser General Public License for more details.                        *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with Targoman. If not, see <http://www.gnu.org/licenses/>.           *
 *                                                                            *
 ******************************************************************************/
/**
 * @author S. Mohammad M. Ziabary <ziabary@targoman.com>
 * @author Behrooz Vedadian <vedadian@targoman.com>
 * @author Saeed Torabzadeh <saeed.torabzadeh@targoman.com>
 */

#include "clsStringBasedProbingModel.h"
#include "libTargomanCommon/Constants.h"

namespace Targoman {
namespace NLPLibs {
namespace TargomanLM {
namespace Private {

using namespace Common;

/**
 * @brief returns probablity of input NGram and maximum order of founded NGram.
 *
 * This function calculates and retrieves probablity of input NGram using maximum founded order of existed NGram in language model and higher order backoff weights.
 * Here is an example of calculating a 5 token input string:
 * Suppose our input string is w1,w2,w3,w4,w5 and we want to calculate the probablity of this string in our 5 Gram language model.
 * First we check whether w1,w2,w3,w4,w5 string exist in language model or not. if it exists the probablity of that returns out.
 * If it doesn't exist but w2,w3,w4,w5 exists, the backoff weight of w1,w2,w3,w4 adds with log probablity of w2,w3,w4,w5.
 * If both w1,w2,w3,w4,w5 and w2,s3,w4,w5 doesn't exist but w3,w4,w5 exists, backoff weight of w1,w2,w3,w4 and w2,w3,w4 and log probablity of w3,w4,w5 adds together, and so on.
 * The maximum order of NGram that is founded in language model and sum of log probablity of founded NGram and backoff of higher order NGram will be the return value of this function.
 * Note that This function calculates this process from unigram to higher order grams.
 *
 *
 * @param[in] _ngram            input NGram in a list of QString.
 * @param[out] _foundedGram     maximum order of NGram that was existed in Hash Table.
 * @return                      returns probablity of input NGram.
 */

LogP_t clsStringBasedProbingModel::lookupNGram(const QStringList& _ngram, quint8& _foundedGram) const
{
    Q_ASSERT(_ngram.size());

    stuProbAndBackoffWeights PB;
    LogP_t      Backoff = Constants::LogP_One;
    LogP_t      Prob = Constants::LogP_Zero;
    QString     NGram = _ngram.last();
    QString     NGram2 = _ngram.last();
    quint8      CurrGram = 0;
    _foundedGram = 1;

    while (true){
        PB = this->getNGramWeights(NGram.toUtf8().constData());
        if (PB.ID > 0){ 
            Prob = PB.Prob;
            Backoff = Constants::LogP_One;
            _foundedGram = CurrGram+1;
        }

        if (++CurrGram >= _ngram.size()){
            break;
        }
        NGram = ((QStringList)_ngram.mid(_ngram.size() - CurrGram - 1)).join(" ");
        NGram2 = ((QStringList)_ngram.mid(_ngram.size() - CurrGram - 1, CurrGram)).join(" ");
        PB = this->getNGramWeights(NGram2.toUtf8().constData());
        if (PB.ID > 0){
            Backoff += PB.Backoff;
        }
    }
    return Prob + Backoff;
}

}
}
}
}
