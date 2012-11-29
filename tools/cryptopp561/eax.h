#ifndef CRYPTOPP_EAX_H
#define CRYPTOPP_EAX_H

#include "authenc.h"
#include "modes.h"
#include "cmac.h"

NAMESPACE_BEGIN(CryptoPP)

//! .
class CRYPTOPP_NO_VTABLE EAX_Base : public AuthenticatedSymmetricCipherBase
{
public:
	// AuthenticatedSymmetricCipher
	std::string AlgorithmName() const
		{return GetMAC().GetCipher().AlgorithmName() + std::string("/EAX");}
	size_t MinKeyLength() const
		{return GetMAC().MinKeyLength();}
	size_t MaxKeyLength() const
		{return GetMAC().MaxKeyLength();}
	size_t DefaultKeyLength() const
		{return GetMAC().DefaultKeyLength();}
	size_t GetValidKeyLength(size_t n) const
		{return GetMAC().GetValidKeyLength(n);}
	bool IsValidKeyLength(size_t n) const
		{return GetMAC().IsValidKeyLength(n);}
	unsigned int OptimalDataAlignment() const
		{return GetMAC().OptimalDataAlignment();}
	IV_Requirement IVRequirement() const
		{return UNIQUE_IV;}
	unsigned int IVSize() const
		{return GetMAC().TagSize();}
	unsigned int MinIVLength() const
		{return 0;}
	unsigned int MaxIVLength() const
		{return UINT_MAX;}
	unsigned int DigestSize() const
		{return GetMAC().TagSize();}
	lword MaxHeaderLength() const
		{return LWORD_MAX;}
	lword MaxMessageLength() const
		{return LWORD_MAX;}

protected:
	// AuthenticatedSymmetricCipherBase
	bool AuthenticationIsOnPlaintext() const
		{return false;}
	unsigned int AuthenticationBlockSize() const
		{return 1;}
	void SetKeyWithoutResync(const byte *userKey, size_t keylength, const NameValuePairs &params);
	void Resync(const byte *iv, size_t len);
	size_t AuthenticateBlocks(const byte *data, size_t len);
	void AuthenticateLastHeaderBlock();
	void AuthenticateLastFooterBlock(byte *mac, size_t macSize);
	SymmetricCipher & AccessSymmetricCipher() {return m_ctr;}
	const CMAC_Base & GetMAC() const {return const_cast<EAX_Base *>(this)->AccessMAC();}
	virtual CMAC_Base & AccessMAC() =0;

	CTR_Mode_ExternalCipher::Encryption m_ctr;
};

//! .
template <class T_BlockCipher, bool T_IsEncryption>
class EAX_Final : public EAX_Base
{
public:
	static std::string StaticAlgorithmName()
		{return T_BlockCipher::StaticAlgorithmName() + std::string("/EAX");}
	bool IsForwardTransformation() const
		{return T_IsEncryption;}

private:
	CMAC_Base & AccessMAC() {return m_cmac;}
	CMAC<T_BlockCipher> m_cmac;
};

#ifdef EAX	// EAX is defined to 11 on GCC 3.4.3, OpenSolaris 8.11
#undef EAX
#endif

/// <a href="http://www.cryptolounge.org/wiki/EAX">EAX</a>
template <class T_BlockCipher>
struct EAX : public AuthenticatedSymmetricCipherDocumentation
{
	typedef EAX_Final<T_BlockCipher, true> Encryption;
	typedef EAX_Final<T_BlockCipher, false> Decryption;
};

NAMESPACE_END

#endif
