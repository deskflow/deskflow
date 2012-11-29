#ifndef CRYPTOPP_DSA_H
#define CRYPTOPP_DSA_H

/** \file
*/

#include "gfpcrypt.h"

NAMESPACE_BEGIN(CryptoPP)

/*! The DSA signature format used by Crypto++ is as defined by IEEE P1363.
  Java uses the DER format, and OpenPGP uses the OpenPGP format. */
enum DSASignatureFormat {DSA_P1363, DSA_DER, DSA_OPENPGP};
/** This function converts between these formats, and returns length of signature in the target format.
	If toFormat == DSA_P1363, bufferSize must equal publicKey.SignatureLength() */
size_t DSAConvertSignatureFormat(byte *buffer, size_t bufferSize, DSASignatureFormat toFormat, 
	const byte *signature, size_t signatureLen, DSASignatureFormat fromFormat);

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY

typedef DSA::Signer DSAPrivateKey;
typedef DSA::Verifier DSAPublicKey;

const int MIN_DSA_PRIME_LENGTH = DSA::MIN_PRIME_LENGTH;
const int MAX_DSA_PRIME_LENGTH = DSA::MAX_PRIME_LENGTH;
const int DSA_PRIME_LENGTH_MULTIPLE = DSA::PRIME_LENGTH_MULTIPLE;

inline bool GenerateDSAPrimes(const byte *seed, size_t seedLength, int &counter, Integer &p, unsigned int primeLength, Integer &q)
	{return DSA::GeneratePrimes(seed, seedLength, counter, p, primeLength, q);}

#endif

NAMESPACE_END

#endif
