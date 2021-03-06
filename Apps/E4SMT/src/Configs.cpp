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
#include "ISO639.h"

namespace Targoman {
namespace Apps {

using namespace Common;
using namespace Common::Configuration;

tmplConfigurable<enuAppMode::Type> gConfigs::Mode(
        gConfigs::appConfig("Mode"),
        "Application working mode",
        enuAppMode::Text2IXML,
        ReturnTrueCrossValidator(),
        "m",
        "ApplicationMode",
        "mode",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<QString> gConfigs::Language(
        gConfigs::appConfig("Language"),
        "Source text language in ISO639 format",
        "",
        [](const Targoman::Common::Configuration::intfConfigurable& _item,QString& _errorMessage){
            if (ISO639isValid(_item.toVariant().toString().toUtf8().constData()))
               return true;
            _errorMessage = _item.toVariant().toString() + " is not in ISO639 format";
            return false;
        },
        "l",
        "LANGUAGE",
        "lang",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<QString> gConfigs::Input(
        gConfigs::appConfig("Input"),
        "One line input which can be plain text or IXML. All other Input/Output configs will be ignored",
        "",
        ReturnTrueCrossValidator(),
        "i",
        "TEXT/IXML",
        "input",
        enuConfigSource::Arg);

tmplConfigurable<FilePath_t>     gConfigs::InputFile(
        gConfigs::appConfig("InputFile"),
        "Input file path to convert. Relative to config file path unless specified as absolute path.",
        "",
        Validators::tmplPathAccessValidator<
        (enuPathAccess::Type)(enuPathAccess::File | enuPathAccess::Readable),
        false>,
        "f",
        "FILEPATH",
        "input-file",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<FilePath_t>     gConfigs::InputDir(
        gConfigs::appConfig("InputDir"),
        "Input directory to convert. Relative to config file path unless specified as absolute path.",
        "",
        Validators::tmplPathAccessValidator<
        (enuPathAccess::Type)(enuPathAccess::Dir | enuPathAccess::Readable),
        false>,
        "d",
        "PATH",
        "input-dir",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<FilePath_t>     gConfigs::OutputPath(
        gConfigs::appConfig("Output"),
        "Output file/directory path to write converted file/files. Defaults to standard output",
        "",
        [] (const intfConfigurable& _item, QString& _errorMessage){
            if (gConfigs::InputDir.value().size())
                return  Validators::tmplPathAccessValidator<
                        (enuPathAccess::Type)(enuPathAccess::Dir | enuPathAccess::Writeatble),false>(
                            _item, _errorMessage);
            else if (gConfigs::InputFile.value().size())
                return  Validators::tmplPathAccessValidator<
                        (enuPathAccess::Type)(enuPathAccess::File | enuPathAccess::Writeatble), false>(
                            _item, _errorMessage);
            else if (_item.toVariant().toString().isEmpty())
                return true;
            _errorMessage="Invalid output selection for simple input text";
            return false;
        },
        "o",
        "PATH",
        "output",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<QWildCard>     gConfigs::IncludePattern(
        gConfigs::appConfig("IncludePattern"),
        "FilePatterns to include when converting (WildCard format)",
        "",
        ReturnTrueCrossValidator(),
        "p",
        "PTRN",
        "pattern",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::Overwrite(
        gConfigs::appConfig("Overwrite"),
        "Overwrite existing files",
        false,
        ReturnTrueCrossValidator(),
        "w",
        "",
        "overwrite",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::Interactive(
        gConfigs::appConfig("Interactive"),
        "Interactive normalization and spell correction",
        false,
        ReturnTrueCrossValidator(),
        "",
        "",
        "interactive",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::NoSpellcorrector(
        gConfigs::appConfig("DontUseSpellcorrector"),
        "Do not use spell corrector",
        false,
        ReturnTrueCrossValidator(),
        "",
        "",
        "no-spellcorrect",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::Recursive(
        gConfigs::appConfig("Recursive"),
        "Recursive directory processing",
        false,
        ReturnTrueCrossValidator(),
        "r",
        "",
        "recursive",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));


tmplConfigurable<bool>     gConfigs::PlainText(
        gConfigs::appConfig("PlainText"),
        "Assume all files as plain text and ignore file suffix",
        false,
        ReturnTrueCrossValidator(),
        "t",
        "",
        "plaintext",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::BreakLines(
        gConfigs::appConfig("BreakLines"),
        "Breaks to lines after converting to either IXML or tokenized text",
        false,
        ReturnTrueCrossValidator(),
        "n",
        "",
        "breaklines",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<bool>     gConfigs::KeepTitles(
        gConfigs::appConfig("KeepTitles"),
        "On XML files keep titles as text",
        false,
        ReturnTrueCrossValidator(),
        "k",
        "",
        "keeptitles",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

tmplConfigurable<FilePath_t>     gConfigs::DTDFilePath(
        gConfigs::appConfig("DTDFilePath"),
        "DTD file used to validate XML Files",
        "",
        Validators::tmplPathAccessValidator<
        (enuPathAccess::Type)(enuPathAccess::Dir | enuPathAccess::Writeatble),
        false>,
        "",
        "DTD_Path",
        "dtd",
        (enuConfigSource::Type)(
            enuConfigSource::Arg  |
            enuConfigSource::File));

}}
/**************************************************************************************************************/
ENUM_CONFIGURABLE_IMPL(Targoman::Apps::enuAppMode)
