/*************************************************************************
 * Copyright © 2012-2015, Targoman.com
 *
 * Published under the terms of TCRL(Targoman Community Research License)
 * You can find a copy of the license file with distributed source or
 * download it from http://targoman.com/License.txt
 *
 *************************************************************************/
/**
  @author S. Mohammad M. Ziabary <smm@ziabary.com>
 @author Behrooz Vedadian <vedadian@gmail.com>
 */

#include <stdlib.h>
#include "libTargomanCommon/Constants.h"
#include "libTargomanCommon/HashFunctions.hpp"
#include "libTargomanCommon/FastOperations.hpp"
#include "libTargomanCommon/clsCmdProgressBar.h"
#include "clsAbstractProbingModel.h"
#include "../Definitions.h"

using namespace Targoman::Common;

namespace Targoman {
namespace NLPLibs {



namespace Private {

clsAbstractProbingModel::clsAbstractProbingModel() : intfBaseModel(enuMemoryModel::Probing)
{
    this->SumLevels = 0;
    this->MaxLevel = 0;
    this->StoredItems = 0;
    this->NGramHashTable = NULL;
}

/**
 * @brief Sets default probability and backoff for unknown words.
 */
void clsAbstractProbingModel::setUnknownWordDefaults(LogP_t _prob, LogP_t _backoff)
{
    this->UnknownWeights.Backoff = _backoff;
    this->UnknownWeights.Prob = _prob;
}

/**
 * @brief This function inserts a string in #NGramHashTable
 *
 * This function first, finds the location for inserting NGram in hash table using sufficient needed level of hashing.
 * Algorithm of this function is in this way:
 * computes its first guess for correct place to insert using murmurHash of string value.
 * While this guessed location is occupied, it guesse another location using higher level of hashing.
 * If after 32 guesses it could not find an empty place for locating it, this string will be inserted in #RemainingHashes which is a QHash variable.
 * When empty place for inserting input NGram found in level "x", hash value of input NGram in level "x+1" and its hash level will be inserted in that location.
 * When a guessed place is occupied, its continue flag will be set.
 * Each cells in hash table is of type stuNGramHash. stuNGramHash have 64 bit variable called stuNGramHash::#HashValueLevel.
 * The first lowest 5 bits of this variable, is for recording hash level (which can save a level between 0-31).
 * The Sixth bit of this variable is for continue flag (which specifies whether another value in hash table, have guessed this occupied place for its location or not).
 * The Seven bit of this variable is for multiIndex flag (which specifies whether value of cell is hash of a multi gram or unigram).
 * The remaining 57 bits of this variable is for recording hash value of Ngram.
 * probability and backoff weight of input NGram will be also recorded in seperate variables, for each cell.
 *
 * @param _ngram    input NGram string.
 * @param _order    order of NGram.
 * @param _prob     probability of NGram
 * @param _backoff  weight of backoff
 */
void clsAbstractProbingModel::insert(const char* _ngram, quint8 _order, LogP_t _prob, LogP_t _backoff)
{
    Hash_t HashLoc, HashValue;
    size_t NGramLen = strlen(_ngram);
    if (_order == 1 && !strcmp(_ngram, LM_UNKNOWN_WORD)){
        this->UnknownWeights.Backoff = _backoff;
        this->UnknownWeights.Prob = _prob;
        return;
    }
    //Our first guess.
    HashLoc = (HashFunctions::murmurHash64(_ngram, NGramLen, 0) % this->HashTableSize) + 1;;
    for (quint8 HashLevel = 1; HashLevel< MAX_HASH_LEVEL; ++HashLevel)
    {
        HashValue = HashFunctions::murmurHash64(_ngram, NGramLen, HashLevel) ;

        if (this->NGramHashTable[HashLoc].HashValueLevel){
            if (this->NGramHashTable[HashLoc].hashValue() == (HashValue & HASHVALUE_CONTAINER) &&
                    this->NGramHashTable[HashLoc].hashLevel() == HashLevel-1)
                throw exLanguageModel(QString("Fatal Collision found on: %1 (%2, %3, %4)").arg(
                                          _ngram).arg(
                                          HashLoc).arg(
                                          this->NGramHashTable[HashLoc].hashValue()).arg(
                                          HashLevel));
            this->NGramHashTable[HashLoc].setContinues();
        }else{
            this->NGramHashTable[HashLoc].HashValueLevel = (HashValue & HASHVALUE_CONTAINER) + HashLevel-1;
            this->NGramHashTable[HashLoc].Prob      = _prob;
            this->NGramHashTable[HashLoc].Backoff   = _backoff;

            this->MaxLevel = qMax(this->MaxLevel, HashLevel);
            this->SumLevels += HashLevel;
            ++this->StoredItems;

            return;
        }
        HashLoc = (HashValue % this->HashTableSize) + 1;
    }

    this->RemainingHashes.insert(_ngram,
                                 stuProbAndBackoffWeights(this->RemainingHashes.size() + this->HashTableSize, _prob,_backoff));
}

/**
 * @brief Initializes and allocates sufficient space for Hash table based on maximum oredr of NGram.
 * @param _maxNGramCount maximum order of NGram.
 */
void clsAbstractProbingModel::init(quint32 _maxNGramCount)
{
    this->HashTableSize = 0;
    for (size_t i =0; i< sizeof(TargomanGoodHashTableSizes)/sizeof(quint32) - 1; i++)
        if (_maxNGramCount < TargomanGoodHashTableSizes[i]){
            this->HashTableSize = TargomanGoodHashTableSizes[i+1];
            break;
        }

    if (this->HashTableSize){
        TargomanInfo(5, "Allocating "<<this->HashTableSize * sizeof(stuNGramHash)<<
                     " Bytes for max "<<this->HashTableSize<<" Items. Curr Items = "<<_maxNGramCount);
        this->NGramHashTable = new stuNGramHash[this->HashTableSize + 1];
        TargomanInfo(5, "Allocated");
        this->NgramCount = _maxNGramCount;
    }else
        throw exLanguageModel("Count of NGrams("+QString::number(_maxNGramCount)+") exeeds valid Table Sizes");
}

void clsAbstractProbingModel::saveBinFile(const QString &_binFilePath, quint8 _order)
{
    QFile BinFile(_binFilePath);
    if (BinFile.open(QFile::WriteOnly) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For writing");
    QDataStream OutputStream(&BinFile);
    clsCmdProgressBar ProgressBar("Storing Bin Data", this->HashTableSize);
    OutputStream << BIN_FILE_HEADER;
    BinFile.write(this->modelHeaderSuffix().toLatin1());
    OutputStream << _order;
    OutputStream << this->HashTableSize;
    OutputStream << this->NgramCount;
    for(Hash_t HashLoc=1; HashLoc<this->HashTableSize; ++HashLoc){
        ProgressBar.setValue(HashLoc);
        if (this->NGramHashTable[HashLoc].hashValue() > 0){
            OutputStream << HashLoc;
            OutputStream << this->NGramHashTable[HashLoc].HashValueLevel;
            OutputStream << this->NGramHashTable[HashLoc].Prob;
            OutputStream << this->NGramHashTable[HashLoc].Backoff;
        }
    }

//    QByteArray HashByteArray;
//    QDataStream HashStream(&HashByteArray, QIODevice::WriteOnly);
//    HashStream << this->RemainingHashes;
//    OutputStream << HashByteArray;
//    BinFile.close();

    ProgressBar.reset("Computing CheckSum", QFileInfo(_binFilePath).size() / 8192);
    QCryptographicHash Crypto(QCryptographicHash::Md5);
    if (BinFile.open(QFile::ReadOnly) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For reading");
    int i=0;
    while(!BinFile.atEnd()){
        ProgressBar.setValue(++i);
        Crypto.addData(BinFile.read(8192)); //Adds data in 8K blocks.
    }
    BinFile.close();
    QByteArray Hash = Crypto.result();

    TargomanInfo(5, QString("BinFile Checksum: ") + Hash.toHex().constData());

    if (BinFile.open(QFile::Append) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For writing");

    BinFile.write(Hash, Hash.size());
    BinFile.close();
}

quint8 clsAbstractProbingModel::loadBinFile(const QString &_binFilePath)
{
    QFile BinFile(_binFilePath);

    if (BinFile.open(QFile::ReadOnly) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For reading");
    QDataStream InputStream(&BinFile);
    QString Header;
    QString Model;
    InputStream >> Header;
    if (Header != BIN_FILE_HEADER)
        throw exLanguageModel("Incompatible bin file");
    quint8 Order;
    try{
        Model = BinFile.read(this->modelHeaderSuffix().size());
        if (Model != this->modelHeaderSuffix())
            throw exLanguageModel(QString("Incompatible Bin File. %1 vs %2").arg(this->modelHeaderSuffix()).arg(Model));
        Order = BinFile.read(sizeof(quint8)).at(0);
        InputStream >> this->HashTableSize;
        InputStream >> this->NgramCount;
    }catch (exLanguageModel &e){
        throw;
    }catch(...){
        throw exLanguageModel("Invalid truncated BinFile");
    }

    if (this->HashTableSize == 0 ||
        this->NgramCount == 0 ||
        this->HashTableSize < this->NgramCount)
        throw exLanguageModel("Invalid bin file");


    clsCmdProgressBar ProgressBar("Computing CheckSum", QFileInfo(_binFilePath).size() / 8192);
    QCryptographicHash Crypto(QCryptographicHash::Md5);
    BinFile.close();
    if (BinFile.open(QFile::ReadOnly) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For reading");
    quint64 FileSize = QFileInfo(_binFilePath).size();
    const quint16 BulkSize = 8192;
    quint16 Remainder = (FileSize - 16) % BulkSize;
    QByteArray MD5Sum;

    try{
        for(quint64 i = 0; i < ((FileSize - 16) / BulkSize); ++i)
            Crypto.addData(BinFile.read(BulkSize));
        Crypto.addData(BinFile.read(Remainder));
        MD5Sum = BinFile.read(16);

    }
    catch(...){
        throw exLanguageModel("Invalid truncated BinFile");
    }
    if (Crypto.result() != MD5Sum)
        throw exLanguageModel(QString("Checksum has failed: %1 vs %2").arg(
                                  MD5Sum.toHex().constData()).arg(
                                  Crypto.result().toHex().constData()));
    BinFile.close();

    if (this->NGramHashTable)
        delete [] this->NGramHashTable;

    TargomanInfo(5, "Allocating "<<this->HashTableSize * sizeof(stuNGramHash)<<
                 " Bytes for max "<<this->HashTableSize<<" Items. Curr Items = "<<this->NgramCount);
    this->NGramHashTable = new stuNGramHash[this->HashTableSize + 1];
    TargomanInfo(5, "Allocated");

    ProgressBar.reset("Loading BinaryLM", this->HashTableSize);
    if (BinFile.open(QFile::ReadOnly) == false)
        throw exLanguageModel("Unable to open <" + _binFilePath + "> For reading");

    InputStream >> Header;
    Model = BinFile.read(this->modelHeaderSuffix().size());
    Order = BinFile.read(sizeof(quint8)).at(0);
    InputStream >> this->HashTableSize;
    InputStream >> this->NgramCount;


    for (Hash_t i; i<this->NgramCount; ++i){
        Hash_t HashLoc;
        InputStream >> HashLoc;
        InputStream >> this->NGramHashTable[HashLoc].HashValueLevel;
        InputStream >> this->NGramHashTable[HashLoc].Prob;
        InputStream >> this->NGramHashTable[HashLoc].Backoff;

        ProgressBar.setValue(i);
    }

//    QByteArray HashByteArray;
//    InputStream >> HashByteArray;
//    QDataStream HashStream(&HashByteArray, QIODevice::ReadOnly);
//    HashStream >> this->RemainingHashes;

    return Order;
}



/**
 * @brief Retrieves probability, backoff weight and ID of input NGram in Hash Table.
 * @param[in] _ngram        input NGram.
 * @param[in] _justSingle   is input NGram is unigram or not.
 * @return returns probability, backoff weight and ID of input NGram in a structure (stuProbAndBackoffWeights).
 */

stuProbAndBackoffWeights clsAbstractProbingModel::getNGramWeights(const char *_ngram, bool _justSingle) const
{
    Hash_t HashLoc, HashValue;
    size_t NGramLen = strlen(_ngram);

    HashLoc = (HashFunctions::murmurHash64(_ngram, NGramLen, 0) % this->HashTableSize) + 1;

    for (quint8 HashLevel = 1; HashLevel< MAX_HASH_LEVEL; ++HashLevel)
    {
        HashValue = HashFunctions::murmurHash64(_ngram, NGramLen, HashLevel);

        if (this->NGramHashTable[HashLoc].hashValue() == (HashValue & HASHVALUE_CONTAINER) &&
                this->NGramHashTable[HashLoc].hashLevel() == HashLevel-1){
            if (_justSingle && this->NGramHashTable[HashLoc].isMultiIndex())
                //Note that we didn't check continue flag for further search on input unigram.
                //That's because in program flow, we first insert unigrams then we insert multigrams.
                //So it is impossible for a unigram to set continue flag of a multiIndex cell.
                return UnknownWeights;
            else
                return stuProbAndBackoffWeights(
                            HashLoc,
                            this->NGramHashTable[HashLoc].Prob,
                            this->NGramHashTable[HashLoc].Backoff);
        }else if (!this->NGramHashTable[HashLoc].continues())
            return UnknownWeights;
        HashLoc = (HashValue % this->HashTableSize) + 1;
    }

    return this->RemainingHashes.value(_ngram, UnknownWeights);
}

}
}
}
