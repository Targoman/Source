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

#ifndef TARGOMAN_CORE_PRIVATE_FEATUREFUNCTIONS_LANGUAGEMODEL_H
#define TARGOMAN_CORE_PRIVATE_FEATUREFUNCTIONS_LANGUAGEMODEL_H

#include "Private/FeatureFunctions/intfFeatureFunction.hpp"

namespace Targoman {
namespace SMT {
namespace Private {
namespace FeatureFunction{

TARGOMAN_ADD_EXCEPTION_HANDLER(exLanguageModel, exFeatureFunction);


class LanguageModel : public intfFeatureFunction
{
public:
    ~LanguageModel(){}

    void initialize(const QString &){}

    Common::Cost_t scoreSearchGraphNode(SearchGraphBuilder::clsSearchGraphNode& _newHypothesisNode) const;

    Common::Cost_t getRestCostForPosition(const Coverage_t& _coverage, size_t _beginPos, size_t endPos) const {
        Q_UNUSED(_coverage);
        Q_UNUSED(_beginPos);
        Q_UNUSED(endPos);
        return 0;
    }

    /**
     * @brief Computes approximate cost to the future cost heuristic
     * @note Either getRestCostForPosition or this function must return 0
     */
    Common::Cost_t getApproximateCost(unsigned _sourceStart,
                                      unsigned _sourceEnd,
                                      const RuleTable::clsTargetRule& _targetRule) const;

    bool nodesHaveSameState(const SearchGraphBuilder::clsSearchGraphNode &_first,
                            const SearchGraphBuilder::clsSearchGraphNode &_second) const;

    inline QStringList columnNames() const{return QStringList();}

    void initRootNode(SearchGraphBuilder::clsSearchGraphNode &_rootNode);

private:
    LanguageModel():
        intfFeatureFunction(this->moduleName(), false)
    {}
    TARGOMAN_DEFINE_SINGLETONMODULE("FeatureFunctions/LanguageModel", LanguageModel);

public:
    Common::Cost_t getLanguageModelCost(const RuleTable::clsTargetRule& _targetRule) const;

private:
    static Common::Configuration::tmplConfigurable<double> ScalingFactor;
};


}
}
}
}

#endif // TARGOMAN_CORE_PRIVATE_FEATUREFUNCTIONS_LANGUAGEMODEL_H
