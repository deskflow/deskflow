// base64.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "base64.h"

NAMESPACE_BEGIN(CryptoPP)

static const byte s_vec[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const byte s_padding = '=';

void Base64Encoder::IsolatedInitialize(const NameValuePairs &parameters)
{
	bool insertLineBreaks = parameters.GetValueWithDefault(Name::InsertLineBreaks(), true);
	int maxLineLength = parameters.GetIntValueWithDefault(Name::MaxLineLength(), 72);

	const char *lineBreak = insertLineBreaks ? "\n" : "";
	
	m_filter->Initialize(CombinedNameValuePairs(
		parameters,
		MakeParameters(Name::EncodingLookupArray(), &s_vec[0], false)
			(Name::PaddingByte(), s_padding)
			(Name::GroupSize(), insertLineBreaks ? maxLineLength : 0)
			(Name::Separator(), ConstByteArrayParameter(lineBreak))
			(Name::Terminator(), ConstByteArrayParameter(lineBreak))
			(Name::Log2Base(), 6, true)));
}

const int *Base64Decoder::GetDecodingLookupArray()
{
	static volatile bool s_initialized = false;
	static int s_array[256];

	if (!s_initialized)
	{
		InitializeDecodingLookupArray(s_array, s_vec, 64, false);
		s_initialized = true;
	}
	return s_array;
}

NAMESPACE_END
