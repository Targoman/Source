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
 */

#ifndef TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGURATION_P_H
#define TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGURATION_P_H

#include <QHash>
#include <QVariant>
#include "Configuration/ConfigManager.h"

namespace Targoman {
namespace Common {
namespace Configuration {
/**
 *  @brief Private section of Targoman::Common module where internal functionalities needed to be obfuscated
 *  are defined
 */
namespace Private {

class clsConfigNetworkServer;

class clsConfigManagerPrivate : public QObject
{
    Q_OBJECT
public:
    clsConfigManagerPrivate(ConfigManager& _parent);
    ~clsConfigManagerPrivate();
    void printHelp(const QString &_license, bool _minimal);
    QList<intfConfigurable*> configItems(const QString& _parent, bool _isRegEX, bool _reportRemote);
    void prepareServer();
    bool isNetworkBased();
    void startNetworkListening();

public:
    /**
     * @brief This is a registry (Map) for all configs and arguments of all programs such as normalizer, the key of
     * this Map, specifies the program and option of that program, and value of this Map, specifies the value of that
     * option.
     */
    QHash<QString, Configuration::intfConfigurable*>    Configs;

    /**
     * @brief ModuleInstantiators
     */
    QHash<QString, stuInstantiator>                     ModuleInstantiators;

    /**
     * @brief In case that we have a config file (.ini file) for arguments of our programs, we put the address of
     * that config file in #ConfigFilePath
     */
    QString                              ConfigFilePath;
    QString                              ConfigFileDir;
    /**
     * @brief This variable will be true if init() function of Configuration class works without any exceptions.
     */
    bool Initialized;

    bool SetPathsRelativeToConfigPath;

    QString ActorUUID;

    ConfigManager& Parent;
    QScopedPointer<clsConfigNetworkServer> ConfigNetServer;
};

}
}
}
}

#endif // TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGURATION_P_H
