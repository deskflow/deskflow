#ifndef CRYPTOPP_BASE32_H
#define CRYPTOPP_BASE32_H

#include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! Converts given data to base 32, the default code is based on draft-ietf-idn-dude-02.txt
/*! To specify alternative code, call Initialize() with EncodingLookupArray parameter. */
class Base32Encoder : public SimpleProxyFilter
{
public:
	Base32Encoder(BufferedTransformation *attachment = NULL, bool uppercase = true, int outputGroupSize = 0, const std::string &separator = ":", const std::string &terminator = "")
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::Uppercase(), uppercase)(Name::GroupSize(), outputGroupSize)(Name::Separator(), ConstByteArrayParameter(separator)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! Decode base 32 data back to bytes, the default code is based on draft-ietf-idn-dude-02.txt
/*! To specify alternative code, call Initialize() with DecodingLookupArray parameter. */
class Base32Decoder : public BaseN_Decoder
{
public:
	Base32Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDefaultDecodingLookupArray(), 5, attachment) {}

	void IsolatedInitialize(const NameValuePairs &parameters);

private:
	static const int * CRYPTOPP_API GetDefaultDecodingLookupArray();
};

NAMESPACE_END

#endif
