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
#include "libTargomanCommon/Logger.h"
#include "libTargomanCommon/exTargomanBase.h"
#include "libTargomanCommon/clsSafeCoreApplication.h"
#include "libTargomanCommon/Macros.h"
#include "libTargomanCommon/clsCmdProgressBar.h"
#include "libTargomanCommon/HashFunctions.hpp"
#include "libTargomanCommon/Configuration/ConfigManager.h"
#include "libTargomanCommon/Configuration/tmplConfigurable.h"
#include "libTargomanCommon/Configuration/Validators.h"
#include "libTargomanCommon/Configuration/clsModuleConfig.hpp"

#include <iostream>
#include <unistd.h>

using namespace Targoman::Common;

static Configuration::tmplConfigurable<qint8> A("/","fjkdfjkdsfjk",123);
static Configuration::tmplConfigurable<QString> B("/s","fjkdfjkdsfjk",123,
                                                  new Configuration::Validators::clsPathValidator(PathAccess::Dir | PathAccess::Readable));

static Configuration::clsModuleConfig  LM("/Modules/LM", "LM Module");

class MLM : Configuration::intfModule{
public:
    MLM() : intfModule("MLM"){}
private:
    static intfModule * instantiator(){
        return new MLM;
    }
    static Configuration::clsModuleRegistrar Registrar;
};

Configuration::clsModuleRegistrar MLM::Registrar("MLM", MLM::instantiator);

class exSample: public exTargomanBase
{
public:
  exSample(const QString& _message, int _line) throw () :
    exTargomanBase(_message, _line)
  {
    this->Message.append(" >;exSample:");
  }
};

TARGOMAN_ADD_EXCEPTION_HANDLER(exSample2, exSample);

class clsSample
{
  public:
  clsSample()
  {
    TargomanDebugLine;
    TargomanDebug(5, "Hello World!");
    TargomanDebug(5,"%s", "Hello World!!");
  }
};

TARGOMAN_DEFINE_ENHANCED_ENUM_BEGIN(enuTest)
Val0,
Val1,
Val2,
Val3
TARGOMAN_DEFINE_ENHANCED_ENUM_STRINGS
"Val0",
"Val1",
"Val2",
"Val3"
TARGOMAN_DEFINE_ENHANCED_ENUM_END


void checkOutput()
{

    TargomanDebugLine
    QString myQString ("Debug QString");
    std::string myString ("Debug String");

    TargomanDebug(6,"DEBUG"<<" Stream"<<1);
    TargomanDebug(5,"%s b=%d","DEBUG",2);
//    TargomanDebug(5,myQString);
//    TargomanDebug(5,"myString");
    TargomanInlineDebug(5,"Checking...");
//    TargomanInlineDebug(5,myQString);
    TargomanFinishInlineDebug(TARGOMAN_COLOR_ERROR,"Ok");

    TargomanWarn(6,"Warn"<<" Stream"<<1);
    TargomanWarn(5,"%s b=%d","Warn",2);

    TargomanInfo(6,"Info"<<" Stream"<<1);
    TargomanInfo(5,"%s b=%d","Info",2);
    TargomanInlineInfo(5,"Checking...");
    TargomanFinishInlineInfo(TARGOMAN_COLOR_ERROR,"Failed");

    TargomanOut(6,"Normal"<<" Stream"<<1);
    TargomanOut(5,"%s b=%d","Normal",2);
    TargomanInlineOut(5,"Checking...");
    TargomanFinishInlineOut(TARGOMAN_COLOR_WARNING,"Warn");

    TargomanHappy(6,"Happy"<<" Stream"<<1);
    TargomanHappy(5,"%s b=%d","Happy",2);

    TargomanError("Error"<<" Stream"<<1);
    TargomanError("%s b=%d","Error",2);
}


int main(int argc, char *argv[])
{
    try{
      Targoman::Common::printLoadedLibs();

      QString ActorUUID;

      TARGOMAN_REGISTER_ACTOR("testLibCommon");
      TARGOMAN_IO_SETTINGS.setFull();
      Targoman::Common::Logger::instance().init("log.log");

      Targoman::Common::TARGOMAN_IO_SETTINGS.ShowColored = false;
      checkOutput();
      Targoman::Common::TARGOMAN_IO_SETTINGS.ShowColored = true;
      checkOutput();
      Targoman::Common::TARGOMAN_IO_SETTINGS.setSilent();
      qDebug("************************* Silented");
      checkOutput();
      qDebug("************************* After Silence");

      TARGOMAN_IO_SETTINGS.setDefault(6, 6);

      checkOutput();

      A.toVariant();

      clsSample Sample;

      clsSafeCoreApplication App(argc, argv);

      TargomanLogWarn(5, " Log Warn");
      TargomanLogInfo(5, " Log Info");
      TargomanLogHappy(5, " Log Happy");
      TargomanLogDebug(5, " Log Debug");
      TargomanLogError(" Log Error");


      try{
        throw exSample2("Hello Wolrd to exception");
        throw exSample("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ", __LINE__);
      }catch (exSample& e){
        TargomanError(" %s", qPrintable(e.what()));
      }catch (exTargomanBase& e){
        TargomanError(" %s", qPrintable(e.what()));
      }

      try{
        throw exTargomanInvalidParameter("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ", __LINE__);
      }catch (exSample& e){
        TargomanError(" %s", qPrintable(e.what()));
      }catch (exTargomanBase& e){
        TargomanError(" %s", qPrintable(e.what()));
      }

      QString Test = "this is a test";
      TargomanHappy(1,HashFunctions::murmurHash32(Test.toUtf8().constData(), Test.length(),1));
      TargomanHappy(1,HashFunctions::murmurHash32(Test.toUtf8().constData(), Test.length(),2));
      TargomanHappy(1,HashFunctions::murmurHash32(Test.toUtf8().constData(), Test.length(),1));

      TargomanHappy(1,HashFunctions::murmurHash64(Test.toUtf8().constData(), Test.length(),1));
      TargomanHappy(1,HashFunctions::murmurHash64(Test.toUtf8().constData(), Test.length(),2));
      TargomanHappy(1,HashFunctions::murmurHash64(Test.toUtf8().constData(), Test.length(),1));
      TargomanHappy(1,HashFunctions::murmurHash64(Test.toUtf8().constData(), Test.length(),1192));

     // Configuration::ConfigManager::instance().init("Sample");
     // Configuration::ConfigManager::instance().init("Sample", QStringList()<<"-h");
      clsCmdProgressBar PB("Test Progress", 10000);
      for (int i=0; i< 10001; i++){
          PB.setValue(i);
          usleep(500);
      }

      return 0;
    }catch(exTargomanBase &e){
        TargomanError(e.what());
    }

/**/
  //  return a.exec();*/
}

