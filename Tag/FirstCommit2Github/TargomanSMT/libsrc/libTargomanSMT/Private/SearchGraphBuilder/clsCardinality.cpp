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

#include "clsCardinality.h"
#include "clsSearchGraph.h"
#include <iostream>

namespace Targoman{
namespace SMT {
namespace Private{
namespace SearchGraphBuilder {

using namespace Common;
using namespace Common::Configuration;


tmplConfigurable<quint16> clsCardinalityHypothesisContainer::MaxCardinalityContainerSize(
        clsSearchGraph::moduleBaseconfig() + "/MaxCardinalityContainerSize",
        "TODO Desc",
        100);
tmplConfigurable<quint8> clsCardinalityHypothesisContainer::PrimaryCoverageShare(
        clsSearchGraph::moduleBaseconfig() + "/PrimaryCoverageShare",
        "TODO Desc",
        0
        );

tmplConfigurable<double> clsCardinalityHypothesisContainer::SearchBeamWidth(
        clsSearchGraph::moduleBaseconfig() + "/SearchBeamWidth",
        "TODO Desc",
        5
        );

size_t clsCardinalityHypothesisContainer::MaxCardinalitySizeLazyPruning = 0;

clsCardinalityHypothesisContainer::clsCardinalityHypothesisContainer() :
    Data(new clsCardinalityHypothesisContainerData)
{ }

bool clsCardinalityHypothesisContainer::insertNewHypothesis(clsSearchGraphNode &_node)
{
    if(_node.getTotalCost() +
            clsCardinalityHypothesisContainer::SearchBeamWidth.value() <
            this->Data->CostLimit)
    {
        this->Data->CostLimit = _node.getTotalCost() +
                clsCardinalityHypothesisContainer::SearchBeamWidth.value();
    }

    const Coverage_t& Coverage = _node.coverage();
    clsLexicalHypothesisContainer& Container = this->Data->LexicalHypothesisContainer[Coverage];

    size_t OldContainerSize = Container.nodes().size();

    bool InsertionDone = Container.insertHypothesis(_node);
    if (InsertionDone){
        if (this->Data->BestLexicalHypothesis == NULL){
            Q_ASSERT(this->Data->WorstLexicalHypothesis == NULL);
            this->Data->BestLexicalHypothesis = &Container;
            this->Data->BestCoverage = Coverage;
            this->Data->WorstLexicalHypothesis = &Container;
            this->Data->WorstCoverage = Coverage;
        } else
            this->localUpdateBestAndWorstNodes(Coverage, Container, _node);
    }

    this->Data->TotalSearchGraphNodeCount += ((qint64)Container.nodes().size() - (qint64)OldContainerSize);

    if(this->Data->TotalSearchGraphNodeCount >
            clsCardinalityHypothesisContainer::MaxCardinalitySizeLazyPruning)
        this->prune();

    return InsertionDone;
}

bool clsCardinalityHypothesisContainer::mustBePruned(Cost_t _cost) const
{
    if(_cost > this->Data->CostLimit)
        return true;
    return false;
}

void clsCardinalityHypothesisContainer::updateWorstNode()
{
    Cost_t  WorstCost = -INFINITY;
    CoverageLexicalHypothesisMap_t::Iterator WorstLexIter = this->Data->LexicalHypothesisContainer.end();
    for(CoverageLexicalHypothesisMap_t::Iterator LexIter = this->Data->LexicalHypothesisContainer.begin();
        LexIter != this->Data->LexicalHypothesisContainer.end();
        ++LexIter){
        //TODO Why may we have empty container
        Q_ASSERT(LexIter->nodes().size());

        if (LexIter->nodes().size()  && WorstCost < LexIter->nodes().last().getTotalCost()){
            WorstCost = LexIter->nodes().last().getTotalCost();
            WorstLexIter = LexIter;
        }
    }

    if(WorstLexIter != this->Data->LexicalHypothesisContainer.end()) {
        this->Data->WorstLexicalHypothesis = &(*WorstLexIter);
        this->Data->WorstCoverage = WorstLexIter.key();
    } else {
        this->Data->WorstLexicalHypothesis = NULL;
        this->Data->WorstCoverage = Coverage_t();
    }
}

void clsCardinalityHypothesisContainer::updateBestAndWorstNodes()
{
    Cost_t  BestCost = INFINITY;
    CoverageLexicalHypothesisMap_t::Iterator BestLexIter = this->Data->LexicalHypothesisContainer.end();
    Cost_t  WorstCost = -INFINITY;
    CoverageLexicalHypothesisMap_t::Iterator WorstLexIter = this->Data->LexicalHypothesisContainer.end();
    for(CoverageLexicalHypothesisMap_t::Iterator LexIter = this->Data->LexicalHypothesisContainer.begin();
        LexIter != this->Data->LexicalHypothesisContainer.end();
        ++LexIter){
        //TODO Why may we have empty container
        Q_ASSERT(LexIter->nodes().size());

        if (LexIter->nodes().size()) {
            if(BestCost > LexIter->nodes().first().getTotalCost()){
                BestCost = LexIter->nodes().first().getTotalCost();
                BestLexIter = LexIter;
            }
            if(WorstCost < LexIter->nodes().last().getTotalCost()){
                WorstCost = LexIter->nodes().last().getTotalCost();
                WorstLexIter = LexIter;
            }
        }
    }

    if(BestLexIter != this->Data->LexicalHypothesisContainer.end()) {
        this->Data->BestLexicalHypothesis = &(*BestLexIter);
        this->Data->BestCoverage = BestLexIter.key();
    } else {
        this->Data->BestLexicalHypothesis = NULL;
        this->Data->BestCoverage = Coverage_t();
    }
    if(WorstLexIter != this->Data->LexicalHypothesisContainer.end()) {
        this->Data->WorstLexicalHypothesis = &(*WorstLexIter);
        this->Data->WorstCoverage = WorstLexIter.key();
    } else {
        this->Data->WorstLexicalHypothesis = NULL;
        this->Data->WorstCoverage = Coverage_t();
    }
}

void clsCardinalityHypothesisContainer::localUpdateBestAndWorstNodes(const Coverage_t& _coverage, clsLexicalHypothesisContainer &_container, const clsSearchGraphNode &_node)
{
    if (_node.isRecombined())
        return;

    if (/* this->Data->BestLexicalHypothesis != NULL && */
            _node.getTotalCost() <
                this->Data->BestLexicalHypothesis->nodes().first().getTotalCost()) {
        this->Data->BestLexicalHypothesis = &_container;
        this->Data->BestCoverage = _coverage;
    }

    if (_node.getTotalCost() > this->Data->WorstLexicalHypothesis->nodes().last().getTotalCost()){
        this->Data->WorstLexicalHypothesis = &_container;
        this->Data->WorstCoverage = _coverage;
    }
}

void clsCardinalityHypothesisContainer::prune()
{
    if(this->Data->TotalSearchGraphNodeCount <= this->MaxCardinalityContainerSize.value()) {
        updateBestAndWorstNodes();
        return;
    }
    QHash<Coverage_t, int> PickedHypothesisCount;
    int TotalSearchGraphNodeCount = 0;
    // First perform pruning respecting the primal share of each coverage if needed
    if(clsCardinalityHypothesisContainer::PrimaryCoverageShare.value() != 0) {
        for(auto LexHypoContainerIter = this->Data->LexicalHypothesisContainer.begin();
            LexHypoContainerIter != this->Data->LexicalHypothesisContainer.end();
            ++LexHypoContainerIter) {
            int PickedFromThisCoverage = qMin(
                        (int)clsCardinalityHypothesisContainer::PrimaryCoverageShare.value(),
                        LexHypoContainerIter->nodes().size()
                        );
            PickedHypothesisCount[LexHypoContainerIter.key()] = PickedFromThisCoverage;
            TotalSearchGraphNodeCount += PickedFromThisCoverage;
        }
    } else {
        // Without primal share, nodes of the lexical hypothesis containers will be
        // chosen only based on their costs, so initially we do not choose any nodes
        // from any of these containers
        for(auto LexHypoContainerIter = this->Data->LexicalHypothesisContainer.begin();
            LexHypoContainerIter != this->Data->LexicalHypothesisContainer.end();
            ++LexHypoContainerIter)
            PickedHypothesisCount[LexHypoContainerIter.key()] = 0;
    }
    // Fill up vacant places if there are any
    while(TotalSearchGraphNodeCount <
            clsCardinalityHypothesisContainer::MaxCardinalityContainerSize.value()) {
        Coverage_t ChosenCoverage;
        Cost_t ChosenNodeTotalCost =
                this->Data->BestLexicalHypothesis->nodes().first().getTotalCost() +
                clsCardinalityHypothesisContainer::SearchBeamWidth.value();
        for(auto HypoContainerIter = this->Data->LexicalHypothesisContainer.begin();
            HypoContainerIter != this->Data->LexicalHypothesisContainer.end();
            ++HypoContainerIter) {
            int NextCoverageNodeIndex = PickedHypothesisCount[HypoContainerIter.key()];
            if(NextCoverageNodeIndex >= HypoContainerIter->nodes().size())
                continue;
            const clsSearchGraphNode& Node = HypoContainerIter->nodes().at(NextCoverageNodeIndex);

            #ifdef TARGOMAN_SHOW_DEBUG
            // Torabzadeh
            if(Node.prevNode().targetRule().toStr() == QStringLiteral("فلسطین است") &&
               Node.targetRule().toStr() == QStringLiteral("که") &&
               Node.coverage() == "11111111101111000000000000"){
                   int a = 2;
                   a++;

                   for(int i = 0; i < HypoContainerIter->nodes().size(); ++i)
                   {
                       const clsSearchGraphNode& Node = HypoContainerIter->nodes().at(i);
                       qDebug() << "(" << Node.prevNode().targetRule().toStr() << ")" << Node.targetRule().toStr() << Node.getTotalCost() - Node.getCost();
                   }
//                   exit(0);
            }
            #endif

            if(Node.getTotalCost() < ChosenNodeTotalCost) {
                ChosenCoverage = HypoContainerIter.key();
                ChosenNodeTotalCost = Node.getTotalCost();
            }
        }
        if(ChosenCoverage.count(true) == 0)
            break;
        PickedHypothesisCount[ChosenCoverage]++;
        TotalSearchGraphNodeCount++;
    }
    // Remove empty containers
    for(auto CoverageIter = PickedHypothesisCount.begin();
        CoverageIter != PickedHypothesisCount.end();
        ++CoverageIter) {
        if(*CoverageIter == 0)
            this->Data->LexicalHypothesisContainer.remove(CoverageIter.key());
        else if (*CoverageIter < this->Data->LexicalHypothesisContainer[CoverageIter.key()].nodes().size()){
            QList<clsSearchGraphNode>& Nodes =  this->Data->LexicalHypothesisContainer[CoverageIter.key()].nodes();

            #ifdef TARGOMAN_SHOW_DEBUG
            // Torabzadeh
            for(int i = *CoverageIter; i < Nodes.size(); ++i){
                if(Nodes.at(i).prevNode().targetRule().toStr() == QStringLiteral("فلسطین است") &&
                   Nodes.at(i).targetRule().toStr() == QStringLiteral("که") &&
                   Nodes.at(i).coverage() == "11111111101111000000000000"){
                       int a = 3;
                       a++;
                }
            }
            #endif

            Nodes.erase(Nodes.begin() + *CoverageIter, Nodes.end());
        }
    }
    // Update best and worst placeholders and the total node count
    this->Data->TotalSearchGraphNodeCount = TotalSearchGraphNodeCount;
    updateBestAndWorstNodes();
    if(this->Data->TotalSearchGraphNodeCount > 0)
        this->Data->CostLimit = this->Data->WorstLexicalHypothesis->nodes().last().getTotalCost();
}

}
}
}
}

