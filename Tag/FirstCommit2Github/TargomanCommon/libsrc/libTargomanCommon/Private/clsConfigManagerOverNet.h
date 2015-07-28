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

#ifndef TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGMANAGEROVERNET_H
#define TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGMANAGEROVERNET_H

#include <QThread>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QTime>
#include "Configuration/tmplConfigurable.h"
#include "clsConfigManager_p.h"

namespace Targoman {
namespace Common {
namespace Configuration {
namespace Private {

TARGOMAN_ADD_EXCEPTION_HANDLER(exConfigurationServer, exConfiguration);

/******************************************************************************/
class clsConfigNetworkServer : private QTcpServer
{
    Q_OBJECT
public:
    clsConfigNetworkServer(clsConfigManagerPrivate& _configManager);
    ~clsConfigNetworkServer();
    void start(bool _justCheck = false);
    bool check();
    bool isActive(){
        return this->ListenPort.value() > 0;
    }

private:
    void incomingConnection(qintptr _socketDescriptor);

private:
    clsConfigManagerPrivate&          ConfigManagerPrivate;
    QString&                          ActorUUID;
 //   QHash<QString,
private:
    static tmplConfigurable<int>      ListenPort;
    static tmplConfigurable<bool>     WaitPortReady;
    static tmplConfigurable<bool>     AdminLocal;
    static tmplConfigurable<int>      MaxSessionTime;
    static tmplConfigurable<int>      MaxIdleTime;
    static tmplConfigurable<quint16>  MaxConnections;
};

/******************************************************************************/
class clsClientThread : public QThread
{
  Q_OBJECT
public:
  clsClientThread(qintptr _socketDescriptor,
                  clsConfigManagerPrivate& _configManager,
                  QObject* _parent);

private:
  void run();
  void sendError(enuReturnType::Type _type, const QString& _message);
  void sendResult(const QString &_data);

private slots:
  void slotReadyRead();
  void slotDisconnected();

signals:
  void error(QTcpSocket::SocketError socketerror);

private:
  qintptr                    SocketDescriptor;
  QString                    ActorName;
  bool                       AllowedToChange;
  bool                       AllowedToView;
  clsConfigManagerPrivate&   ConfigManagerPrivate;
  QString&                   ActorUUID;
  QTcpSocket*                Socket;
};

}
}
}
}
#endif // TARGOMAN_COMMON_CONFIGURATION_PRIVATE_CLSCONFIGMANAGEROVERNET_H
