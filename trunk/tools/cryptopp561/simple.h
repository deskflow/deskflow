// simple.h - written and placed in the public domain by Wei Dai
/*! \file
 	Simple non-interface classes derived from classes in cryptlib.h.
*/

#ifndef CRYPTOPP_SIMPLE_H
#define CRYPTOPP_SIMPLE_H

#include "cryptlib.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
template <class DERIVED, class BASE>
class CRYPTOPP_NO_VTABLE ClonableImpl : public BASE
{
public:
	Clonable * Clone() const {return new DERIVED(*static_cast<const DERIVED *>(this));}
};

//! _
template <class BASE, class ALGORITHM_INFO=BASE>
class CRYPTOPP_NO_VTABLE AlgorithmImpl : public BASE
{
public:
	static std::string CRYPTOPP_API StaticAlgorithmName() {return ALGORITHM_INFO::StaticAlgorithmName();}
	std::string AlgorithmName() const {return ALGORITHM_INFO::StaticAlgorithmName();}
};

//! _
class CRYPTOPP_DLL InvalidKeyLength : public InvalidArgument
{
public:
	explicit InvalidKeyLength(const std::string &algorithm, size_t length) : InvalidArgument(algorithm + ": " + IntToString(length) + " is not a valid key length") {}
};

//! _
class CRYPTOPP_DLL InvalidRounds : public InvalidArgument
{
public:
	explicit InvalidRounds(const std::string &algorithm, unsigned int rounds) : InvalidArgument(algorithm + ": " + IntToString(rounds) + " is not a valid number of rounds") {}
};

// *****************************

//! _
template <class T>
class CRYPTOPP_NO_VTABLE Bufferless : public T
{
public:
	bool IsolatedFlush(bool hardFlush, bool blocking) {return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE Unflushable : public T
{
public:
	bool Flush(bool completeFlush, int propagation=-1, bool blocking=true)
		{return ChannelFlush(DEFAULT_CHANNEL, completeFlush, propagation, blocking);}
	bool IsolatedFlush(bool hardFlush, bool blocking)
		{assert(false); return false;}
	bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true)
	{
		if (hardFlush && !InputBufferIsEmpty())
			throw CannotFlush("Unflushable<T>: this object has buffered input that cannot be flushed");
		else 
		{
			BufferedTransformation *attached = this->AttachedTransformation();
			return attached && propagation ? attached->ChannelFlush(channel, hardFlush, propagation-1, blocking) : false;
		}
	}

protected:
	virtual bool InputBufferIsEmpty() const {return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE InputRejecting : public T
{
public:
	struct InputRejected : public NotImplemented
		{InputRejected() : NotImplemented("BufferedTransformation: this object doesn't allow input") {}};

	// shouldn't be calling these functions on this class
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
		{throw InputRejected();}
	bool IsolatedFlush(bool, bool) {return false;}
	bool IsolatedMessageSeriesEnd(bool) {throw InputRejected();}

	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking)
		{throw InputRejected();}
	bool ChannelMessageSeriesEnd(const std::string &, int, bool) {throw InputRejected();}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE CustomFlushPropagation : public T
{
public:
	virtual bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) =0;

private:
	bool IsolatedFlush(bool hardFlush, bool blocking) {assert(false); return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE CustomSignalPropagation : public CustomFlushPropagation<T>
{
public:
	virtual void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1) =0;

private:
	void IsolatedInitialize(const NameValuePairs &parameters) {assert(false);}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE Multichannel : public CustomFlushPropagation<T>
{
public:
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true)
		{return this->ChannelFlush(DEFAULT_CHANNEL, hardFlush, propagation, blocking);}
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true)
		{return this->ChannelMessageSeriesEnd(DEFAULT_CHANNEL, propagation, blocking);}
	byte * CreatePutSpace(size_t &size)
		{return this->ChannelCreatePutSpace(DEFAULT_CHANNEL, size);}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
		{return this->ChannelPut2(DEFAULT_CHANNEL, begin, length, messageEnd, blocking);}
	size_t PutModifiable2(byte *inString, size_t length, int messageEnd, bool blocking)
		{return this->ChannelPutModifiable2(DEFAULT_CHANNEL, inString, length, messageEnd, blocking);}

//	void ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1)
//		{PropagateMessageSeriesEnd(propagation, channel);}
	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size)
		{size = 0; return NULL;}
	bool ChannelPutModifiable(const std::string &channel, byte *inString, size_t length)
		{this->ChannelPut(channel, inString, length); return false;}

	virtual size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking) =0;
	size_t ChannelPutModifiable2(const std::string &channel, byte *begin, size_t length, int messageEnd, bool blocking)
		{return ChannelPut2(channel, begin, length, messageEnd, blocking);}

	virtual bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true) =0;
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE AutoSignaling : public T
{
public:
	AutoSignaling(int propagation=-1) : m_autoSignalPropagation(propagation) {}

	void SetAutoSignalPropagation(int propagation)
		{m_autoSignalPropagation = propagation;}
	int GetAutoSignalPropagation() const
		{return m_autoSignalPropagation;}

private:
	int m_autoSignalPropagation;
};

//! A BufferedTransformation that only contains pre-existing data as "output"
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Store : public AutoSignaling<InputRejecting<BufferedTransformation> >
{
public:
	Store() : m_messageEnd(false) {}

	void IsolatedInitialize(const NameValuePairs &parameters)
	{
		m_messageEnd = false;
		StoreInitialize(parameters);
	}

	unsigned int NumberOfMessages() const {return m_messageEnd ? 0 : 1;}
	bool GetNextMessage();
	unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=DEFAULT_CHANNEL) const;

protected:
	virtual void StoreInitialize(const NameValuePairs &parameters) =0;

	bool m_messageEnd;
};

//! A BufferedTransformation that doesn't produce any retrievable output
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Sink : public BufferedTransformation
{
public:
	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true)
		{transferBytes = 0; return 0;}
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const
		{return 0;}
};

class CRYPTOPP_DLL BitBucket : public Bufferless<Sink>
{
public:
	std::string AlgorithmName() const {return "BitBucket";}
	void IsolatedInitialize(const NameValuePairs &parameters) {}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
		{return 0;}
};

NAMESPACE_END

#endif
