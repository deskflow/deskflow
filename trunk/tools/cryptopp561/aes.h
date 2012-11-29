#ifndef CRYPTOPP_AES_H
#define CRYPTOPP_AES_H

#include "rijndael.h"

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.cryptolounge.org/wiki/AES">AES</a> winner, announced on 10/2/2000
DOCUMENTED_TYPEDEF(Rijndael, AES);

typedef RijndaelEncryption AESEncryption;
typedef RijndaelDecryption AESDecryption;

NAMESPACE_END

#endif
