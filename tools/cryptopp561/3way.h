#ifndef CRYPTOPP_THREEWAY_H
#define CRYPTOPP_THREEWAY_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
struct ThreeWay_Info : public FixedBlockSize<12>, public FixedKeyLength<12>, public VariableRounds<11>
{
	static const char *StaticAlgorithmName() {return "3-Way";}
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#3-Way">3-Way</a>
class ThreeWay : public ThreeWay_Info, public BlockCipherDocumentation
{
	class CRYPTOPP_NO_VTABLE Base : public BlockCipherImpl<ThreeWay_Info>
	{
	public:
		void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params);

	protected:
		unsigned int m_rounds;
		FixedSizeSecBlock<word32, 3> m_k;
	};

	class CRYPTOPP_NO_VTABLE Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class CRYPTOPP_NO_VTABLE Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherFinal<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherFinal<DECRYPTION, Dec> Decryption;
};

typedef ThreeWay::Encryption ThreeWayEncryption;
typedef ThreeWay::Decryption ThreeWayDecryption;

NAMESPACE_END

#endif
