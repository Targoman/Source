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
 */

#include "Configs.h"
#include "libTargomanCommon/Configuration/Validators.hpp"

namespace Targoman {
namespace Apps {

using namespace Common;
using namespace Common::Configuration;

gConfigs::stuServer::stuServer(const QString &_basePath) :
    Host(_basePath + "Host",
         "Host or IP Address of Server",
         "",
         Common::Configuration::ReturnTrueCrossValidator,
         "","","",
         Common::Configuration::enuConfigSource::File
         ),
    Port(_basePath + "Port",
         "Server port to connect to",
         1,
         Common::Configuration::ReturnTrueCrossValidator,
         "","","",
         Common::Configuration::enuConfigSource::File
         ),
    AbsoluteScore(_basePath + "AbsoluteScore",
         "Absolute score of Server used in priority computations",
        0,
         Validators::tmplNumericValidator<quint8,0,100>,
         "","","",
         Common::Configuration::enuConfigSource::File
         ),
    Active(_basePath + "Active",
         "Host or IP Address of Server",
         false,
         Common::Configuration::ReturnTrueCrossValidator,
         "","","",
         Common::Configuration::enuConfigSource::File
         ),
    UserName(_basePath + "Username",
         "Administration user",
         "TargomanLB",
         Common::Configuration::ReturnTrueCrossValidator,
         "","","",
         Common::Configuration::enuConfigSource::File,
         false
         ),
    Password(_basePath + "Password",
         "Administration password",
         "",
         Common::Configuration::ReturnTrueCrossValidator,
         "","","",
         Common::Configuration::enuConfigSource::File,
         false
         ),
    Statistics(gConfigs::stuServer::stuStatistics(_basePath))
{}

gConfigs::stuServer::stuStatistics::stuStatistics(const QString &_basePath) :
    Load1MinPercent(_basePath + "Stats/Load1Min",
                    "Load average on 1min basis divided by count of cores",
                    -1,
                    Common::Configuration::ReturnTrueCrossValidator,
                    "","","",
                    Common::Configuration::enuConfigSource::ReadOnly
                    ),
    Load15MinPercent(_basePath + "Stats/Load15Min",
                     "Load average on 15min basis divided by count of cores",
                     -1,
                     Common::Configuration::ReturnTrueCrossValidator,
                     "","","",
                     Common::Configuration::enuConfigSource::ReadOnly
                     ),
    FreeMemoryPercent(_basePath + "Stats/FreeMemory",
                      "FreeMemory Available on server divided by total amount of memory",
                      -1,
                      Common::Configuration::ReturnTrueCrossValidator,
                      "","","",
                      Common::Configuration::enuConfigSource::ReadOnly
                      ),
    TranslationQueuePercent(_basePath + "Stats/TranslationQueue",
                            "Translation Queue usage",
                            -1,
                            Common::Configuration::ReturnTrueCrossValidator,
                            "","","",
                            Common::Configuration::enuConfigSource::ReadOnly
                            )
  {}

tmplConfigurableMultiMap<gConfigs::stuServer> gConfigs::TranslationServers(
        "TranslationServers",
        "List of valid translation servers to connect to them separated by their translation direction");

QString ActorUUID;
}
}
