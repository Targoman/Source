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

#include <functional>

#include "clsSearchGraph.h"
#include "../GlobalConfigs.h"
#include "Private/Proxies/intfLMSentenceScorer.hpp"
#include "Private/SpecialTokenHandler/SpecialTokensRegistry.hpp"
#include "Private/SpecialTokenHandler/OOVHandler/OOVHandler.h"
#include <iostream>
#include <sstream>


#define PBT_MAXIMUM_COST 1e200

namespace Targoman{
namespace SMT {
namespace Private{
namespace SearchGraphBuilder {

using namespace Common;
using namespace Common::Configuration;
using namespace RuleTable;
using namespace Proxies;
using namespace InputDecomposer;
using namespace SpecialTokenHandler;
using namespace SpecialTokenHandler::OOV;

tmplConfigurable<quint8> clsSearchGraph::HardReorderingJumpLimit(
        clsSearchGraph::moduleBaseconfig() + "/HardReorderingJumpLimit",
        "TODO Desc",
        6,
        [] (const intfConfigurable& _item, QString&) {
    clsCardinalityHypothesisContainer::setHardReorderingJumpLimit(
            _item.toVariant().Int
            );
    return true;
});

tmplConfigurable<quint8> clsSearchGraph::ReorderingConstraintMaximumRuns(
        clsSearchGraph::moduleBaseconfig() + "/ReorderingConstraintMaximumRuns",
        "IBM1 reordering constraint",
        2);
tmplConfigurable<bool>   clsSearchGraph::DoComputePositionSpecificRestCosts(
        clsSearchGraph::moduleBaseconfig() + "/DoComputePositionSpecificRestCosts",
        "TODO Desc",
        true);
tmplConfigurable<quint8> clsPhraseCandidateCollectionData::MaxTargetPhraseCount(
        clsSearchGraph::moduleBaseconfig() + "/MaxTargetPhraseCount",
        "TODO Desc",
        100);

tmplConfigurable<bool>   clsSearchGraph::DoPrunePreInsertion(
        clsSearchGraph::moduleBaseconfig() + "/PrunePreInsertion",
        "TODO Desc",
        true);

FeatureFunction::intfFeatureFunction*  clsSearchGraph::pPhraseTable = NULL;
RuleTable::intfRuleTable*              clsSearchGraph::pRuleTable = NULL;
RuleTable::clsRuleNode*                clsSearchGraph::UnknownWordRuleNode;

/**********************************************************************************/
clsSearchGraph::clsSearchGraph(const Sentence_t& _sentence):
    Data(new clsSearchGraphData(_sentence))
{
    this->collectPhraseCandidates();
    this->decode();
}
/**
 * @brief Loads rule and phrase tables, inititializes all feature functions and sets #UnknownWordRuleNode.
 * @param _configFilePath Address of config file.
 */
void clsSearchGraph::init(const QString& _configFilePath)
{
    clsSearchGraph::pRuleTable = gConfigs.RuleTable.getInstance<intfRuleTable>();

    clsSearchGraph::pRuleTable->initializeSchema();

    //InvalidTargetRuleData has been marshalled here because it depends on loading RuleTable
    RuleTable::InvalidTargetRuleData = new RuleTable::clsTargetRuleData;

    //pInvalidTargetRule has been marshalled here because it depends on instantiation of InvalidTargetRuleData
    RuleTable::pInvalidTargetRule = new RuleTable::clsTargetRule;
    //InvalidSearchGraphNodeData has been marshalled here because it depends on initialization of gConfigs
    InvalidSearchGraphNodeData = new clsSearchGraphNodeData;
    pInvalidSearchGraphNode = new clsSearchGraphNode;

    foreach (FeatureFunction::intfFeatureFunction* FF, gConfigs.ActiveFeatureFunctions)
        FF->initialize(_configFilePath);
    clsSearchGraph::pRuleTable->loadTableData();
    clsSearchGraph::pPhraseTable = gConfigs.ActiveFeatureFunctions.value("PhraseTable");


    RulesPrefixTree_t::pNode_t Node = clsSearchGraph::pRuleTable->prefixTree().rootNode();
    if(Node->isInvalid())
        throw exSearchGraph("Invalid empty Rule Table");

    Node = Node->follow(Constants::SrcVocabUnkWordIndex); //Search for UNKNOWN word Index
    if (Node->isInvalid())
        throw exSearchGraph("No Rule defined for UNKNOWN word");


    clsSearchGraph::UnknownWordRuleNode = new clsRuleNode(Node->getData(), true);

    /// @note As a result of aligning some words to NULL by general word aligners, we need to take care of
    ///       tokens that have a word index but only contribute to multi-word phrases. These will cause
    ///       malfunction of OOV handler module as it will assume these words have translations by themselves
    for(auto TokenIter = gConfigs.SourceVocab.begin(); TokenIter != gConfigs.SourceVocab.end(); ++TokenIter) {
//    for(auto TokenIter = gConfigs.SourceVocab.find("Tape"); TokenIter != gConfigs.SourceVocab.end(); ++TokenIter) {
        clsRuleNode& SingleWordRuleNode =
                clsSearchGraph::pRuleTable->prefixTree().getOrCreateNode(
                    QList<WordIndex_t>() << TokenIter.value()
                    )->getData();
        if(SingleWordRuleNode.isInvalid())
            SingleWordRuleNode.detachInvalidData();
        if(SingleWordRuleNode.targetRules().isEmpty())
            SingleWordRuleNode.targetRules().append(
                        OOVHandler::instance().generateTargetRules(TokenIter.key())
                        );
//        break;
   }

}

void clsSearchGraph::extendSourcePhrase(const QList<WordIndex_t>& _wordIndexes,
                                        INOUT QList<RulesPrefixTree_t::pNode_t>& _prevNodes,
                                        QList<clsRuleNode>& _ruleNodes)
{
    QList<RulesPrefixTree_t::pNode_t> NextNodes;
    foreach(RulesPrefixTree_t::pNode_t PrevNode, _prevNodes) {
        foreach(WordIndex_t WordIndex, _wordIndexes) {
            RulesPrefixTree_t::pNode_t NextNode = PrevNode->follow(WordIndex);
            if(NextNode->isInvalid() == false) {
                _ruleNodes.append(NextNode->getData());
                NextNodes.append(NextNode);
            }
        }
    }
    _prevNodes = NextNodes;
}

/**
 * @brief Looks up prefix tree for all phrases that matches with some parts of input sentence and stores them in the
 * PhraseCandidateCollections of #Data. This function also calculates maximum length of matching source phrase with phrase table.
 */
void clsSearchGraph::collectPhraseCandidates()
{
    // TODO: When looking for phrases containing IXML tags, search both for the tagged version
    // and surface form version, e.g. "I ate <num>3</num>" => search for "I ate <num/>" and "I ate 3"
    this->Data->MaxMatchingSourcePhraseCardinality = 0;
    for (size_t FirstPosition = 0; FirstPosition < (size_t)this->Data->Sentence.size(); ++FirstPosition) {
        this->Data->PhraseCandidateCollections.append(QVector<clsPhraseCandidateCollection>(this->Data->Sentence.size() - FirstPosition));
        QList<RulesPrefixTree_t::pNode_t> PrevNodes =
                QList<RulesPrefixTree_t::pNode_t>() << this->pRuleTable->prefixTree().rootNode();

        if(true /* On 1-grams */)
        {
            QList<clsRuleNode> RuleNodes;
            this->extendSourcePhrase(this->Data->Sentence.at(FirstPosition).wordIndexes(),
                                     PrevNodes,
                                     RuleNodes);

            foreach(WordIndex_t WordIndex, this->Data->Sentence.at(FirstPosition).wordIndexes()) {
                clsRuleNode SpecialRuleNode = SpecialTokensRegistry::instance().getRuleNode(WordIndex);
                if(SpecialRuleNode.isInvalid() == false)
                    RuleNodes.append(SpecialRuleNode);
            }

            if(RuleNodes.isEmpty())
                RuleNodes.append(*clsSearchGraph::UnknownWordRuleNode);

            this->Data->MaxMatchingSourcePhraseCardinality = qMax(this->Data->MaxMatchingSourcePhraseCardinality, 1);

            this->Data->PhraseCandidateCollections[FirstPosition][0] = clsPhraseCandidateCollection(FirstPosition, FirstPosition + 1, RuleNodes);

        }

        //Max PhraseTable order will be implicitly checked by follow
        for (size_t LastPosition = FirstPosition + 1; LastPosition < (size_t)this->Data->Sentence.size() ; ++LastPosition){

            QList<clsRuleNode> RuleNodes;
            extendSourcePhrase(this->Data->Sentence.at(LastPosition).wordIndexes(), PrevNodes, RuleNodes);

            if (RuleNodes.isEmpty())
                break; // appending next word breaks phrase lookup

            this->Data->PhraseCandidateCollections[FirstPosition][LastPosition - FirstPosition] = clsPhraseCandidateCollection(FirstPosition, LastPosition + 1, RuleNodes);
            this->Data->MaxMatchingSourcePhraseCardinality = qMax(this->Data->MaxMatchingSourcePhraseCardinality,
                                                                      (int)(LastPosition - FirstPosition + 1));
        }
    }

#ifdef TARGOMAN_SHOW_DEBUG
    // Torabzadeh
    for (size_t FirstPosition = 0; FirstPosition < (size_t)this->Data->Sentence.size(); ++FirstPosition) {
        for (size_t LastPosition = FirstPosition ; LastPosition < (size_t)this->Data->Sentence.size() ; ++LastPosition) {
            for(int i = 0; i < this->Data->PhraseCandidateCollections[FirstPosition][LastPosition - FirstPosition].targetRules().size(); ++i) {
                std::cout << "BEGIN- " << this->Data->PhraseCandidateCollections[FirstPosition][LastPosition - FirstPosition].targetRules().at(i).toStr().toUtf8().constData()
                          << " IDX[" << FirstPosition << ", " << LastPosition-FirstPosition << "] "
                          << " -END" << std::endl;
            }
        }
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << std::endl;
    }
    exit(0);
#endif

}


/**
 * @brief This function checks IBM1 Constraints.
 *
 * Checks whether number of zeros before last 1 in the input coverage is less than #ReorderingConstraintMaximumRuns or not.
 * @param _newCoverage
 * @return
 */
bool clsSearchGraph::conformsIBM1Constraint(const Coverage_t& _newCoverage)
{
    //Find last bit set then check how many bits are zero before this.
    for(int i=_newCoverage.size() - 1; i>=0; --i)
        if(_newCoverage.testBit(i)){
            size_t CountOfPrevZeros = _newCoverage.count(false) + i - _newCoverage.size() + 1;
            return (CountOfPrevZeros <= this->ReorderingConstraintMaximumRuns.value());
        }
    return true;
}

bool clsSearchGraph::conformsHardReorderingJumpLimit(const Coverage_t &_coverage,
                                                            size_t _endPos)
{
    int FirstEmptyPosition = _coverage.size();
    for(int i = 0; i < _coverage.size(); ++i)
        if(_coverage.testBit(i) == false) {
            FirstEmptyPosition = i;
            break;
        }
    if(FirstEmptyPosition == _coverage.size())
        return true;
    size_t JumpWidth = qAbs((int)_endPos - FirstEmptyPosition);
    return JumpWidth <= clsSearchGraph::HardReorderingJumpLimit.value();
}

#ifdef TARGOMAN_SHOW_DEBUG
float roundDouble(double inputNumber)
{
    return (float)((int(100 * inputNumber + 0.5))/100.0);
}
#endif


/**
 * @brief This is the main function that performs the decoding process.
 *
 * This function first initializes the rest cost matrix in order to be able to calculate rest costs in each path of translation.
 * Then in order to seperate all search graph nodes, this function seperates them by their number of translated words. This notion is called cardinality.
 * For each cardinality loops over all posible previous cardinalities and for each one of them finds all nodes with those cardinalities.
 * In the next step, these founded previous nodes, based on their translation coverage, will be expanded to make new search
 * graph nodes with NewCardinality size. In this process, some of new Nodes will be prunned to have valid and more
 * promising search graph nodes.
 *
 * @return Returns whether it was able to find a translation or not.
 */
bool clsSearchGraph::decode()
{
    this->Data->HypothesisHolder.clear();
    this->Data->HypothesisHolder.resize(this->Data->Sentence.size() + 1);

    this->initializeRestCostsMatrix();


    for (int NewCardinality = 1; NewCardinality <= this->Data->Sentence.size(); ++NewCardinality){

        //int PrunedByIBMConstraint = 0;
        int PrunedByHardReorderingJumpLimit = 0;
        int PrunedPreInsertion = 0;

        bool IsFinal = (NewCardinality == this->Data->Sentence.size());
        int MinPrevCardinality = qMax(NewCardinality - this->Data->MaxMatchingSourcePhraseCardinality, 0);

        clsCardinalityHypothesisContainer& CurrCardHypoContainer =
                this->Data->HypothesisHolder[NewCardinality];

        for (int PrevCardinality = MinPrevCardinality;
             PrevCardinality < NewCardinality; ++PrevCardinality) {

            unsigned short NewPhraseCardinality = NewCardinality - PrevCardinality;

            clsCardinalityHypothesisContainer& PrevCardHypoContainer =
                    this->Data->HypothesisHolder[PrevCardinality];

            //This happens when we have for ex. 2 bi-grams and a quad-gram but no similar 3-gram. due to bad training
            if(PrevCardHypoContainer.isEmpty()) {
                TargomanLogWarn(1, "Previous cardinality is empty. (PrevCard: " << PrevCardinality << ", CurrentCard: " << NewCardinality << ")");
                continue;
            }

            for(CoverageLexicalHypothesisMap_t::Iterator PrevCoverageIter = PrevCardHypoContainer.lexicalHypotheses().begin();
                PrevCoverageIter != PrevCardHypoContainer.lexicalHypotheses().end();
                ++PrevCoverageIter){


                const Coverage_t& PrevCoverage = PrevCoverageIter.key();
                clsLexicalHypothesisContainer& PrevLexHypoContainer = PrevCoverageIter.value();

                Q_ASSERT(PrevCoverage.count(true) == PrevCardinality);
                Q_ASSERT(PrevLexHypoContainer.nodes().size());


                // This can be removed if training has been done properly and we have a sane phrase table
                if (PrevLexHypoContainer.nodes().isEmpty()){
                    //TODO: We must have this warning log in release mode also
#ifdef TARGOMAN_SHOW_DEBUG
                    TargomanLogWarn(1, "PrevLexHypoContainer is empty. PrevCard: " << PrevCardinality
                                  << "PrevCov: " << PrevCoverage);
#endif
                    continue;
                }

                for (size_t NewPhraseBeginPos = 0;
                     NewPhraseBeginPos <= (size_t)this->Data->Sentence.size() - NewPhraseCardinality;
                     ++NewPhraseBeginPos){
                    size_t NewPhraseEndPos = NewPhraseBeginPos + NewPhraseCardinality;

                    // Skip if phrase coverage is not compatible with previous sentence coverage
                    bool SkipStep = false;
                    for (size_t i= NewPhraseBeginPos; i<NewPhraseEndPos; ++i)
                        if (PrevCoverage.testBit(i)){
                            SkipStep = true;
                            break;
                        }
                    if (SkipStep)
                        continue;//TODO if NewPhraseCardinality has not contigeous place breaK

                    Coverage_t NewCoverage(PrevCoverage);
                    for (size_t i=NewPhraseBeginPos; i<NewPhraseEndPos; ++i)
                        NewCoverage.setBit(i);

                    /*
                    if (this->conformsIBM1Constraint(NewCoverage) == false){
                        ++PrunedByIBMConstraint;
                        continue;
                    }
                    */

                    if (this->conformsHardReorderingJumpLimit(NewCoverage, NewPhraseEndPos) == false){
                        ++PrunedByHardReorderingJumpLimit;
                        continue;
                    }

                    clsPhraseCandidateCollection& PhraseCandidates =
                            this->Data->PhraseCandidateCollections[NewPhraseBeginPos][NewPhraseCardinality - 1];

                    //There is no rule defined in rule table for current phrase
                    if (PhraseCandidates.isInvalid())
                        continue; //TODO If there are no more places to fill after this startpos break

                    Cost_t RestCost =  this->calculateRestCost(NewCoverage, NewPhraseBeginPos, NewPhraseEndPos);

                    foreach (const clsSearchGraphNode& PrevLexHypoNode, PrevLexHypoContainer.nodes()) {


                        size_t MaxCandidates = qMin((int)PhraseCandidates.usableTargetRuleCount(),
                                                    PhraseCandidates.targetRules().size());
                        for(size_t i = 0; i<MaxCandidates; ++i){

                            const clsTargetRule& CurrentPhraseCandidate = PhraseCandidates.targetRules().at(i);


                            clsSearchGraphNode NewHypoNode(PrevLexHypoNode,
                                                           NewPhraseBeginPos,
                                                           NewPhraseEndPos,
                                                           NewCoverage,
                                                           CurrentPhraseCandidate,
                                                           IsFinal,
                                                           RestCost);
#ifdef TARGOMAN_SHOW_DEBUG
                            // Torabzadeh
                            if(//NewHypoNode.prevNode().targetRule().toStr() == QStringLiteral("") &&
                               NewHypoNode.targetRule().toStr() == QStringLiteral("به یک") /*&&
                               NewCardinality == 1 &&
                               NewCoverage == "000001000000"*/){
                                   for(int i = 0; i < PhraseCandidates.targetRules().size(); ++i) {
                                       std::cout << PhraseCandidates.targetRules().at(i).toStr().toUtf8().constData() << std::endl;
                                   }
                                   int a = 2;
                                   a++;
                            }
                            if(NewHypoNode.prevNode().targetRule().toStr() == QStringLiteral("فلسطین در") &&
                               NewHypoNode.targetRule().toStr() == QStringLiteral("آن") &&
                               NewCardinality == 13 &&
                               NewCoverage == "11111111101111000000000000"){
                                   int a = 5;
                                   a++;
                            }
#endif

                            // If current NewHypoNode is worse than worst stored node ignore it
                            if (clsSearchGraph::DoPrunePreInsertion.value() &&
                                CurrCardHypoContainer.mustBePruned(NewHypoNode.getTotalCost())){
                                ++PrunedPreInsertion;
                                continue;
                            }

                            if(CurrCardHypoContainer.insertNewHypothesis(NewHypoNode)) {
                                // Log insertion of hypothesis here if it is needed
                            }
                        }
                    }//foreach PrevLexHypoNode
                }//for NewPhraseBeginPos
            }//for PrevCoverageIter
        }//for PrevCardinality
        CurrCardHypoContainer.finlizePruningAndcleanUp();

#ifdef TARGOMAN_SHOW_DEBUG
        // Vedadian
        //*
        auto car2str = [] (int _cardinality) {
            QString result;
            for(int i = 0; i < 5; ++i) {
                result = ('0' + _cardinality % 10) + result;
                _cardinality /= 10;
            }
            return result;
        };
        auto cov2str = [] (const Coverage_t _coverage) {
            QString result;
            QTextStream stream;
            stream.setString(&result);
            stream << _coverage;
            return result;
        };
        if(CurrCardHypoContainer.lexicalHypotheses().size() != 0) {
            for(auto Iterator = CurrCardHypoContainer.lexicalHypotheses().end() - 1;
                true;
                --Iterator) {
                const QList<clsSearchGraphNode>& Nodes = Iterator->nodes();
                foreach(const clsSearchGraphNode& SelectedNode, Nodes) {
                    std::stringstream Stream;
                    Stream << SelectedNode.getTotalCost() << "\t";
                    Stream << "Cardinality:  ";
                    Stream << car2str(NewCardinality).toUtf8().constData();
                    Stream << "  Coverage:  " << cov2str(SelectedNode.coverage()).toUtf8().constData();
                    Stream << "  Cost:  " << SelectedNode.getCost()
                           << " , RestCost: " << SelectedNode.getTotalCost() - SelectedNode.getCost()
                           << " , Str: (" << SelectedNode.prevNode().targetRule().toStr().toUtf8().constData()
                           << ")" << SelectedNode.targetRule().toStr().toUtf8().constData() << std::endl;
                    std::cout << Stream.str().c_str();
                    //TargomanDebug(5, QString::fromUtf8(Stream.str().c_str()));
                }
                if(Iterator == CurrCardHypoContainer.lexicalHypotheses().begin())
                    break;
            }
        }
        std::cout << "\n\n\n";
        //TargomanDebug(5,"\n\n\n");
        //*/
#endif
    }//for NewCardinality

    Coverage_t FullCoverage;
    FullCoverage.fill(1, this->Data->Sentence.size());

    if(this->Data->HypothesisHolder[this->Data->Sentence.size()].lexicalHypotheses().size() > 0 &&
            this->Data->HypothesisHolder[this->Data->Sentence.size()][FullCoverage].nodes().isEmpty() == false)
    {
        this->Data->GoalNode = &this->Data->HypothesisHolder[this->Data->Sentence.size()][FullCoverage].nodes().first();
        this->Data->HypothesisHolder[this->Data->Sentence.size()][FullCoverage].finalizeRecombination();
        return true;
    } else {
        static clsSearchGraphNode InvalidGoalNode;
        this->Data->GoalNode = &InvalidGoalNode;
        // TODO: We need to have this log in release mode also
#ifdef TARGOMAN_SHOW_DEBUG
        TargomanLogWarn(1, "No translation option for: " << this->Data->Sentence);
#endif
        return false;
    }
}

/**
 * @brief Initializes rest cost matrix
 *
 * For every possible range of words of input sentence, finds approximate cost of every feature fucntions, then
 * tries to reduce that computed rest cost if sum of rest cost of splited phrase is less than whole phrase rest cost.
 */
void clsSearchGraph::initializeRestCostsMatrix()
{
    this->Data->RestCostMatrix.resize(this->Data->Sentence.size());
    for (int SentenceStartPos=0; SentenceStartPos<this->Data->Sentence.size(); ++SentenceStartPos)
        this->Data->RestCostMatrix[SentenceStartPos].fill(
                PBT_MAXIMUM_COST,
                this->Data->Sentence.size() - SentenceStartPos);

    for(size_t FirstPosition = 0; FirstPosition < (size_t)this->Data->Sentence.size(); ++FirstPosition){
        size_t MaxLength = qMin(this->Data->Sentence.size() - FirstPosition,
                                (size_t)this->Data->MaxMatchingSourcePhraseCardinality);
        for(size_t Length = 1; Length <= MaxLength; ++Length){
            this->Data->RestCostMatrix[FirstPosition][Length - 1]  = this->Data->PhraseCandidateCollections[FirstPosition][Length-1].bestApproximateCost();
        }
    }

    for(size_t Length = 2; Length <= (size_t)this->Data->Sentence.size(); ++Length)
        for(size_t FirstPosition = 0; FirstPosition + Length <= (size_t)this->Data->Sentence.size(); ++FirstPosition)
            for(size_t SplitPosition = 1; SplitPosition < Length; ++SplitPosition){
                Cost_t SumSplit = this->Data->RestCostMatrix[FirstPosition][SplitPosition - 1] +
                        this->Data->RestCostMatrix[FirstPosition + SplitPosition][Length - 1 - SplitPosition];
                this->Data->RestCostMatrix[FirstPosition][Length - 1]  =
                        qMin(
                            this->Data->RestCostMatrix[FirstPosition][Length - 1],
                            SumSplit);
            }
}

/**
 * @brief This function approximates rest cost of translation for every feature function.
 * This approximation is based on covered word of transltion and begin and end position of source sentence words.
 * @param _coverage Covered word for translation.
 * @param _beginPos start postion of source sentence.
 * @param _endPos end position of source sentence
 * @note _beginPos and _endPos helps us to infer previous node coverage.
 * @return returns approximate cost of rest cost.
 */

Cost_t clsSearchGraph::calculateRestCost(const Coverage_t& _coverage, size_t _beginPos, size_t _endPos) const
{
    Cost_t RestCosts = 0.0;
    size_t StartPosition = 0;
    size_t Length = 0;

    for(size_t i=0; i < (size_t)_coverage.size(); ++i)
        if(_coverage.testBit(i) == false){ // start of contiguous zero bits.
            if(Length == 0)
                StartPosition = i;
            ++Length;
        }else if(Length){ // end of contiguous zero bits.
            RestCosts += this->Data->RestCostMatrix[StartPosition][Length-1];
            Length = 0;
        }
    if(Length)
        RestCosts += this->Data->RestCostMatrix[StartPosition][Length-1];

    if(clsSearchGraph::DoComputePositionSpecificRestCosts.value()) {
        foreach(FeatureFunction::intfFeatureFunction* FF, gConfigs.ActiveFeatureFunctions.values()) {
            if(FF->canComputePositionSpecificRestCost())
                RestCosts += FF->getRestCostForPosition(_coverage, _beginPos, _endPos);
        }
    }
    return RestCosts;
}

clsPhraseCandidateCollectionData::clsPhraseCandidateCollectionData(size_t _beginPos, size_t _endPos, const QList<clsRuleNode> &_ruleNodes)
{
    foreach(const clsRuleNode& RuleNode, _ruleNodes)
        this->TargetRules.append(RuleNode.targetRules());

    this->UsableTargetRuleCount = qMin(
                (int)clsPhraseCandidateCollectionData::MaxTargetPhraseCount.value(),
                this->TargetRules.size()
                );

    // TODO: Maybe sorting can be added here
    this->BestApproximateCost = INFINITY;
    // _observationHistogramSize must be taken care of to not exceed this->TargetRules.size()
    for(int Count = 0; Count < this->UsableTargetRuleCount; ++Count) {
        clsTargetRule& TargetRule = this->TargetRules[Count];
        // Compute the approximate cost for current target rule
        Cost_t ApproximateCost = 0;
        foreach (FeatureFunction::intfFeatureFunction* FF , gConfigs.ActiveFeatureFunctions)
            if(FF->canComputePositionSpecificRestCost() == false) {
                Cost_t Cost = FF->getApproximateCost(_beginPos, _endPos, TargetRule);
                ApproximateCost += Cost;
            }
        this->BestApproximateCost = qMin(this->BestApproximateCost, ApproximateCost);
    }
}

}
}
}
}




/*************************************************
* TODO PREMATURE OPTIMIZATION that does not work properly
*                 size_t StartLookingPos = 0;
size_t NeededSpace = NewPhraseCardinality;
while(StartLookingPos <= (size_t)this->Data->Sentence.size() - NewPhraseCardinality){
    size_t LookingPos = StartLookingPos;
    while (LookingPos < PrevCoverage.size()) {
        if (PrevCoverage.testBit(LookingPos)){
            NeededSpace = NewPhraseCardinality;
        }else{
            --NeededSpace;
            if(NeededSpace == 0)
                break;
        }
        ++LookingPos;
    }

    if (NeededSpace > 0)
        break;

    NeededSpace = 1;
    StartLookingPos = LookingPos+1;
    size_t NewPhraseBeginPos = LookingPos - NewPhraseCardinality + 1;
    size_t NewPhraseEndPos = NewPhraseBeginPos + NewPhraseCardinality;
*************************************************/
