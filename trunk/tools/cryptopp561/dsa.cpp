// dsa.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "dsa.h"
#include "nbtheory.h"

NAMESPACE_BEGIN(CryptoPP)

size_t DSAConvertSignatureFormat(byte *buffer, size_t bufferSize, DSASignatureFormat toFormat, const byte *signature, size_t signatureLen, DSASignatureFormat fromFormat)
{
	Integer r, s;
	StringStore store(signature, signatureLen);
	ArraySink sink(buffer, bufferSize);

	switch (fromFormat)
	{
	case DSA_P1363:
		r.Decode(store, signatureLen/2);
		s.Decode(store, signatureLen/2);
		break;
	case DSA_DER:
	{
		BERSequenceDecoder seq(store);
		r.BERDecode(seq);
		s.BERDecode(seq);
		seq.MessageEnd();
		break;
	}
	case DSA_OPENPGP:
		r.OpenPGPDecode(store);
		s.OpenPGPDecode(store);
		break;
	}

	switch (toFormat)
	{
	case DSA_P1363:
		r.Encode(sink, bufferSize/2);
		s.Encode(sink, bufferSize/2);
		break;
	case DSA_DER:
	{
		DERSequenceEncoder seq(sink);
		r.DEREncode(seq);
		s.DEREncode(seq);
		seq.MessageEnd();
		break;
	}
	case DSA_OPENPGP:
		r.OpenPGPEncode(sink);
		s.OpenPGPEncode(sink);
		break;
	}

	return (size_t)sink.TotalPutLength();
}

bool DSA::GeneratePrimes(const byte *seedIn, unsigned int g, int &counter,
						  Integer &p, unsigned int L, Integer &q, bool useInputCounterValue)
{
	assert(g%8 == 0);

	SHA sha;
	SecByteBlock seed(seedIn, g/8);
	SecByteBlock U(SHA::DIGESTSIZE);
	SecByteBlock temp(SHA::DIGESTSIZE);
	SecByteBlock W(((L-1)/160+1) * SHA::DIGESTSIZE);
	const int n = (L-1) / 160;
	const int b = (L-1) % 160;
	Integer X;

	sha.CalculateDigest(U, seed, g/8);

	for (int i=g/8-1, carry=true; i>=0 && carry; i--)
		carry=!++seed[i];

	sha.CalculateDigest(temp, seed, g/8);
	xorbuf(U, temp, SHA::DIGESTSIZE);

	U[0] |= 0x80;
	U[SHA::DIGESTSIZE-1] |= 1;
	q.Decode(U, SHA::DIGESTSIZE);

	if (!IsPrime(q))
		return false;

	int counterEnd = useInputCounterValue ? counter+1 : 4096;

	for (int c = 0; c < counterEnd; c++)
	{
		for (int k=0; k<=n; k++)
		{
			for (int i=g/8-1, carry=true; i>=0 && carry; i--)
				carry=!++seed[i];
			if (!useInputCounterValue || c == counter)
				sha.CalculateDigest(W+(n-k)*SHA::DIGESTSIZE, seed, g/8);
		}
		if (!useInputCounterValue || c == counter)
		{
			W[SHA::DIGESTSIZE - 1 - b/8] |= 0x80;
			X.Decode(W + SHA::DIGESTSIZE - 1 - b/8, L/8);
			p = X-((X % (2*q))-1);

			if (p.GetBit(L-1) && IsPrime(p))
			{
				counter = c;
				return true;
			}
		}
	}
	return false;
}

NAMESPACE_END

#endif
