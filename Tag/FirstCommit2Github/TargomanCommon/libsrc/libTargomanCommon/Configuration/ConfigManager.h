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

#ifndef TARGOMAN_COMMON_CONFIGMANAGER_H
#define TARGOMAN_COMMON_CONFIGMANAGER_H

#include <cfloat>
#include "libTargomanCommon/exTargomanBase.h"
#include "libTargomanCommon/Configuration/intfConfigurable.hpp"
#include "libTargomanCommon/Configuration/intfConfigurableModule.hpp"
#include "libTargomanCommon/Types.h"
#include "libTargomanCommon/JSONConversationProtocol.h"

namespace Targoman {
namespace Common {
/**
 * @brief Configuration module with many interfaces and templates to ease application configuration
 */
namespace Configuration {

namespace Private {
class clsConfigManagerPrivate;
class intfConfigurablePrivate;
}

TARGOMAN_ADD_EXCEPTION_HANDLER(exConfiguration, exTargomanBase);

/**
 * @brief The ConfigManager class is the manager class for configurables data.
 * Currently it will just manage Arguments and config file
 */
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    ~ConfigManager();
    static inline ConfigManager& instance(){
        static ConfigManager* Instance = NULL;
        return *(Q_LIKELY(Instance) ? Instance : (Instance = new ConfigManager));
    }

    static inline QString moduleName(){return "ConfigManager";}

    void init(const QString &_license, const QStringList &_arguments = QStringList(), bool _minimal = false);
    void save2File(const QString&  _fileName, bool _backup);
    void addConfig(const QString _path, intfConfigurable* _item);
    void addModuleInstantiaor(const QString _name, const stuInstantiator& _instantiator);
    QVariant getConfig(const QString& _path, const QVariant &_default = QVariant()) const;
    void setValue(const QString& _path, const QVariant &_value) const;
    fpModuleInstantiator_t getInstantiator(const QString& _name) const;
    QString configFilePath();
    QString configFileDir();
    QString getAbsolutePath(const QString &_path);
    bool isNetworkManagable();

public slots:
    void startAdminServer();

private:
    ConfigManager();
    Q_DISABLE_COPY(ConfigManager)

signals:
    void sigValidateAgent(INOUT QString&        _user,
                          const QString&        _pass,
                          const QString&        _ip,
                          OUTPUT bool&          _canView,
                          OUTPUT bool&          _canChange);
    void sigPing(Targoman::Common::stuPong& _pong);

private:
    QScopedPointer<Private::clsConfigManagerPrivate> pPrivate;

    friend class Targoman::Common::Configuration::Private::intfConfigurablePrivate;
};

}
}
}
#endif // TARGOMAN_COMMON_CONFIGMANAGER_H
