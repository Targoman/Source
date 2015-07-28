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

#include "clsJanePlainRuleTable.h"
#include "libTargomanCommon/Logger.h"
#include "libTargomanCommon/Configuration/Validators.hpp"
#include "libTargomanCommon/CompressedStream/clsCompressedInputStream.h"
#include "libTargomanCommon/clsCmdProgressBar.h"

#include "Private/FeatureFunctions/PhraseTable/PhraseTable.h"
#include "Private/FeatureFunctions/LexicalReordering/LexicalReordering.h"

namespace Targoman {
namespace SMT {
namespace Private {
namespace RuleTable {

enum {
    janeFormatCostsPosition = 0,        /// First part of jane phrase table corresponds to feature values. Feature values are seperated with space in this part
    janeFormatLeftHandSidePosition,     /// Second part of jane phrase table specifies whether phrase is hirarchichal (S) or not (X).
    janeFormatSourcePosition,           /// Third part of jane phrase table specifies source phrase. Word strings are seperated with space in this part.
    janeFormatTargetPosition,           /// Fourth part of jane phrase table specifies target phrase. Word strings are seperated with space in this part.
    janeFormatCountPosition,            /// Fifth part of jane phrase table is not important for us
    janeFormatStandardNumberOfFields    /// Sixth and next parts of jane phrase table are correspond to additional features like lexiacal reordering. Name of additional feature is first field and other fields in each part are feature values.
};


using namespace Common;
using namespace Common::Configuration;
using namespace Common::CompressedStream;
using namespace FeatureFunction;

TARGOMAN_REGISTER_MODULE(clsJanePlainRuleTable);

#define FFCONFIG_KEY_IDENTIFIER "Key"

tmplConfigurable<FilePath_t> clsJanePlainRuleTable::FilePath(
        clsJanePlainRuleTable::baseConfigPath() + "/FilePath",
        "FilePath where phrase table is stored",
        "",
        ConditionalPathValidator(
            gConfigs.RuleTable.toVariant().toString() == clsJanePlainRuleTable::moduleName(),
            enuPathAccess::File | enuPathAccess::Readable)
        );

tmplConfigurable<QString> clsJanePlainRuleTable::PhraseCostNames(
        clsJanePlainRuleTable::baseConfigPath() + "/CostNames",
        "CostsNames as defined in Jane config File",
        "s2t,t2s,ibm1s2t,ibm1t2s,phrasePenalty,wordPenalty,s2tRatio,t2sRatio,cnt1,cnt2,cnt3");

QList<tmplConfigurable<QString>> clsJanePlainRuleTable::FeatureFunctions = {
    tmplConfigurable<QString>(clsJanePlainRuleTable::baseConfigPath() +
    "/FeatureFunctions/" + FeatureFunction::LexicalReordering::moduleName() + FFCONFIG_KEY_IDENTIFIER,
    "Abbreviation used for each Feature function in the rule table file",
    "lrm")
};

clsJanePlainRuleTable::clsJanePlainRuleTable(quint64 _instanceID)  :
    intfRuleTable(this->moduleName(), _instanceID)
{}

clsJanePlainRuleTable::~clsJanePlainRuleTable()
{
    this->unregister();
}


/**
 * @brief clsJanePlainRuleTable::initializeSchema Just loads first line of phrase table to set column names.
 */
void clsJanePlainRuleTable::initializeSchema()
{
    TargomanLogInfo(5, "Loading Jane plain text rule set schema from: " + this->FilePath.value());

    this->PrefixTree.reset(new RulesPrefixTree_t());
    QStringList ColumnNames = clsJanePlainRuleTable::PhraseCostNames.value().split(",");

    clsCompressedInputStream InputStream(clsJanePlainRuleTable::FilePath.value().toStdString());

    if(InputStream.peek() < 0)
        throw exJanePhraseTable("Empty phrase table.");

    std::string Line;
    getline(InputStream, Line);
    if (Line == "")
        throw exJanePhraseTable("Empty first line.");

    QStringList Fields = QString::fromUtf8(Line.c_str()).split("#");

    if (Fields.size() < 3)
        throw exJanePhraseTable(QString("Bad file format in first line : %2").arg(Line.c_str()));


    PhraseTable::setColumnNames(ColumnNames);

    bool Accepted = false;
    for (unsigned AdditionalFieldIndex = janeFormatStandardNumberOfFields;
         AdditionalFieldIndex < (size_t)Fields.size();
         ++AdditionalFieldIndex) {
        QStringList AdditionalField = Fields.at(AdditionalFieldIndex).split(" ", QString::SkipEmptyParts);
        QString FieldName = AdditionalField.first();
        for(int i = 0; i< clsJanePlainRuleTable::FeatureFunctions.size(); ++i){
            const tmplConfigurable<QString>& FFConfig = clsJanePlainRuleTable::FeatureFunctions.at(i);
            if (FFConfig.value() == FieldName){
                QString FFModuleName = FFConfig.configPath().split("/").last();
                FFModuleName.truncate(FFModuleName.size() - sizeof(FFCONFIG_KEY_IDENTIFIER) + 1);
                if (gConfigs.ActiveFeatureFunctions.value(FFModuleName)->columnNames().size() != AdditionalField.size() - 1)
                    throw exJanePhraseTable("Invalid count of costs for field: " + FFModuleName);
                ColumnNames.append(gConfigs.ActiveFeatureFunctions.value(FFModuleName)->columnNames());
                Accepted = true;
                this->AcceptedAdditionalFieldsIndexes.append(AdditionalFieldIndex);
                break;
            }
        }
        if (Accepted == false){
            TargomanWarn(5,"Unsupported Additional field <" + FieldName + "> ignored!!!");
        }

        clsTargetRule::setColumnNames(ColumnNames);
    }
    TargomanLogInfo(5, "Jane plain text rule set schema loaded. ");
}

/**
 * @brief clsJanePlainRuleTable::loadTableData Loads phrase table from file and adds each phrase (rule) to pefix tree.
 */
void clsJanePlainRuleTable::loadTableData()
{
    TargomanLogInfo(5, "Loading Jane plain text rule set from: " + this->FilePath.value());

    this->PrefixTree.reset(new RulesPrefixTree_t());
    QStringList ColumnNames = clsJanePlainRuleTable::PhraseCostNames.value().split(",");
    size_t      PhraseCostsCount = ColumnNames.size();

    clsCmdProgressBar ProgressBar("Loading RuleTable");

    clsCompressedInputStream InputStream(clsJanePlainRuleTable::FilePath.value().toStdString());
    size_t RulesRead = 0;

    while (InputStream.peek() >= 0){
        std::string Line;
        getline(InputStream, Line);
        if (Line == "")
            continue;
        ++RulesRead;
        ProgressBar.setValue(RulesRead);

        QStringList Fields = QString::fromUtf8(Line.c_str()).split("#");

        if (Fields.size() < 3)
            throw exJanePhraseTable(QString("Bad file format in line %1 : %2").arg(RulesRead).arg(Line.c_str()));

        if (Fields[janeFormatLeftHandSidePosition] == "S") // we ignore hierachichal phrases.
            continue;

        if (Fields[janeFormatCostsPosition].isEmpty()){
            TargomanWarn(5,"Ignoring phrase with empty target side at line: " + QString::number(RulesRead));
            continue;
        }

        QStringList PhraseCostsFields = Fields[janeFormatCostsPosition].split(" ", QString::SkipEmptyParts);
        if ((size_t)PhraseCostsFields.size() < PhraseCostsCount){
            TargomanWarn(5,"Invalid count of costs at line: " + QString::number(RulesRead));
            continue;
        }

        this->addRule(PhraseCostsFields, Fields, this->AcceptedAdditionalFieldsIndexes, RulesRead);
    }
    TargomanLogInfo(5, "Jane plain text rule set loaded. ");
}

/**
 * @brief clsJanePlainRuleTable::addRule    Adds phrases to prefix tree.
 * @param _phraseCosts                      Feature values.
 * @param _allFields                        All fields of input rule.
 * @param _acceptedAdditionalFields         Additional accepted fields like lexical reordering model.
 * @param _ruleNumber                       line number of rule table.
 */
void clsJanePlainRuleTable::addRule(const QStringList& _phraseCosts,
                                    const QStringList &_allFields,
                                    const QList<size_t>& _acceptedAdditionalFields,
                                    size_t _ruleNumber)
{
    Q_UNUSED(_ruleNumber)

    QList<Cost_t>      Costs;
    foreach(const QString& Cost, _phraseCosts)
        Costs.append(Cost.toDouble());

    QList<WordIndex_t> SourcePhrase;
    foreach(const QString& Word, _allFields[janeFormatSourcePosition].split(" ", QString::SkipEmptyParts)){
        WordIndex_t WordIndex = gConfigs.SourceVocab.value(Word, Constants::SrcVocabUnkWordIndex);
        if (WordIndex == Constants::SrcVocabUnkWordIndex && Word != "<unknown-word>"){
            WordIndex = gConfigs.SourceVocab.size() + 1;
            gConfigs.SourceVocab.insert(Word, WordIndex);
        }
        SourcePhrase.append(WordIndex);
    }


    QList<WordIndex_t> TargetPhrase;
    foreach(const QString& Word, _allFields[janeFormatTargetPosition].split(" ", QString::SkipEmptyParts))
        TargetPhrase.append(gConfigs.EmptyLMScorer->getWordIndex(Word));

    foreach(size_t FieldIndex, _acceptedAdditionalFields){
        const QStringList& FieldCosts = _allFields.at(FieldIndex).split(" ", QString::SkipEmptyParts);
        for (size_t i = 1; i<(size_t)FieldCosts.size(); ++i)
            Costs.append(FieldCosts.at(i).toDouble());
    }

    clsTargetRule TargetRule(TargetPhrase, Costs);

    clsRuleNode& RuleNode = this->PrefixTree->getOrCreateNode(SourcePhrase)->getData();
    if(RuleNode.isInvalid())
        RuleNode.detachInvalidData();
    RuleNode.targetRules().append(TargetRule);
}

}
}
}
}
