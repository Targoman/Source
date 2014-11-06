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

#include "libTargomanCommon/CmdIO.h"
#include "libLanguageModel/clsLanguageModel.h"
#include "libLanguageModel/clsLMSentenceScorer.h"

#include <QDebug>

using namespace Targoman::NLPLibs;

int main(int argc, char *argv[])
{
    clsLanguageModel LM;
    qDebug()<<"Order = "<<LM.init("./test.arpa");

    clsLMSentenceScorer SS(LM);
    QString Sentence = "i would look beyond";
//also would consider higher looking
    foreach (const QString& Word, Sentence)
        qDebug()<<"Prob ["<<Word<<"]:"<<SS.wordProb(Word);
}


