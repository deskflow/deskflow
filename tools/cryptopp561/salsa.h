// salsa.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_SALSA_H
#define CRYPTOPP_SALSA_H

#include "strciphr.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
struct Salsa20_Info : public VariableKeyLength<32, 16, 32, 16, SimpleKeyingInterface::UNIQUE_IV, 8>
{
	static const char *StaticAlgorithmName() {return "Salsa20";}
};

class CRYPTOPP_NO_VTABLE Salsa20_Policy : public AdditiveCipherConcretePolicy<word32, 16>
{
protected:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void OperateKeystream(KeystreamOperation operation, byte *output, const byte *input, size_t iterationCount);
	void CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length);
	bool CipherIsRandomAccess() const {return true;}
	void SeekToIteration(lword iterationCount);
#if CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X64
	unsigned int GetAlignment() const;
	unsigned int GetOptimalBlockSize() const;
#endif

	FixedSizeAlignedSecBlock<word32, 16> m_state;
	int m_rounds;
};

/// <a href="http://www.cryptolounge.org/wiki/Salsa20">Salsa20</a>, variable rounds: 8, 12 or 20 (default 20)
struct Salsa20 : public Salsa20_Info, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<Salsa20_Policy, AdditiveCipherTemplate<> >, Salsa20_Info> Encryption;
	typedef Encryption Decryption;
};

//! _
struct XSalsa20_Info : public FixedKeyLength<32, SimpleKeyingInterface::UNIQUE_IV, 24>
{
	static const char *StaticAlgorithmName() {return "XSalsa20";}
};

class CRYPTOPP_NO_VTABLE XSalsa20_Policy : public Salsa20_Policy
{
public:
	void CipherSetKey(const NameValuePairs &params, const byte *key, size_t length);
	void CipherResynchronize(byte *keystreamBuffer, const byte *IV, size_t length);

protected:
	FixedSizeSecBlock<word32, 8> m_key;
};

/// <a href="http://www.cryptolounge.org/wiki/XSalsa20">XSalsa20</a>, variable rounds: 8, 12 or 20 (default 20)
struct XSalsa20 : public XSalsa20_Info, public SymmetricCipherDocumentation
{
	typedef SymmetricCipherFinal<ConcretePolicyHolder<XSalsa20_Policy, AdditiveCipherTemplate<> >, XSalsa20_Info> Encryption;
	typedef Encryption Decryption;
};

NAMESPACE_END

#endif
