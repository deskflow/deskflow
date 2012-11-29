#ifndef CRYPTOPP_FILTERS_H
#define CRYPTOPP_FILTERS_H

//! \file

#include "simple.h"
#include "secblock.h"
#include "misc.h"
#include "smartptr.h"
#include "queue.h"
#include "algparam.h"
#include <deque>

NAMESPACE_BEGIN(CryptoPP)

/// provides an implementation of BufferedTransformation's attachment interface
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Filter : public BufferedTransformation, public NotCopyable
{
public:
	Filter(BufferedTransformation *attachment = NULL);

	bool Attachable() {return true;}
	BufferedTransformation *AttachedTransformation();
	const BufferedTransformation *AttachedTransformation() const;
	void Detach(BufferedTransformation *newAttachment = NULL);

	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true);
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true);

protected:
	virtual BufferedTransformation * NewDefaultAttachment() const;
	void Insert(Filter *nextFilter);	// insert filter after this one

	virtual bool ShouldPropagateMessageEnd() const {return true;}
	virtual bool ShouldPropagateMessageSeriesEnd() const {return true;}

	void PropagateInitialize(const NameValuePairs &parameters, int propagation);

	size_t Output(int outputSite, const byte *inString, size_t length, int messageEnd, bool blocking, const std::string &channel=DEFAULT_CHANNEL);
	size_t OutputModifiable(int outputSite, byte *inString, size_t length, int messageEnd, bool blocking, const std::string &channel=DEFAULT_CHANNEL);
	bool OutputMessageEnd(int outputSite, int propagation, bool blocking, const std::string &channel=DEFAULT_CHANNEL);
	bool OutputFlush(int outputSite, bool hardFlush, int propagation, bool blocking, const std::string &channel=DEFAULT_CHANNEL);
	bool OutputMessageSeriesEnd(int outputSite, int propagation, bool blocking, const std::string &channel=DEFAULT_CHANNEL);

private:
	member_ptr<BufferedTransformation> m_attachment;
	
protected:
	size_t m_inputPosition;
	int m_continueAt;
};

struct CRYPTOPP_DLL FilterPutSpaceHelper
{
	// desiredSize is how much to ask target, bufferSize is how much to allocate in m_tempSpace
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, size_t minSize, size_t desiredSize, size_t &bufferSize)
	{
		assert(desiredSize >= minSize && bufferSize >= minSize);
		if (m_tempSpace.size() < minSize)
		{
			byte *result = target.ChannelCreatePutSpace(channel, desiredSize);
			if (desiredSize >= minSize)
			{
				bufferSize = desiredSize;
				return result;
			}
			m_tempSpace.New(bufferSize);
		}

		bufferSize = m_tempSpace.size();
		return m_tempSpace.begin();
	}
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, size_t minSize)
		{return HelpCreatePutSpace(target, channel, minSize, minSize, minSize);}
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, size_t minSize, size_t bufferSize)
		{return HelpCreatePutSpace(target, channel, minSize, minSize, bufferSize);}
	SecByteBlock m_tempSpace;
};

//! measure how many byte and messages pass through, also serves as valve
class CRYPTOPP_DLL MeterFilter : public Bufferless<Filter>
{
public:
	MeterFilter(BufferedTransformation *attachment=NULL, bool transparent=true)
		: m_transparent(transparent) {Detach(attachment); ResetMeter();}

	void SetTransparent(bool transparent) {m_transparent = transparent;}
	void AddRangeToSkip(unsigned int message, lword position, lword size, bool sortNow = true);
	void ResetMeter();
	void IsolatedInitialize(const NameValuePairs &parameters) {ResetMeter();}

	lword GetCurrentMessageBytes() const {return m_currentMessageBytes;}
	lword GetTotalBytes() {return m_totalBytes;}
	unsigned int GetCurrentSeriesMessages() {return m_currentSeriesMessages;}
	unsigned int GetTotalMessages() {return m_totalMessages;}
	unsigned int GetTotalMessageSeries() {return m_totalMessageSeries;}

	byte * CreatePutSpace(size_t &size)
		{return AttachedTransformation()->CreatePutSpace(size);}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);
	size_t PutModifiable2(byte *inString, size_t length, int messageEnd, bool blocking);
	bool IsolatedMessageSeriesEnd(bool blocking);

private:
	size_t PutMaybeModifiable(byte *inString, size_t length, int messageEnd, bool blocking, bool modifiable);
	bool ShouldPropagateMessageEnd() const {return m_transparent;}
	bool ShouldPropagateMessageSeriesEnd() const {return m_transparent;}

	struct MessageRange
	{
		inline bool operator<(const MessageRange &b) const	// BCB2006 workaround: this has to be a member function
			{return message < b.message || (message == b.message && position < b.position);}
		unsigned int message; lword position; lword size;
	};

	bool m_transparent;
	lword m_currentMessageBytes, m_totalBytes;
	unsigned int m_currentSeriesMessages, m_totalMessages, m_totalMessageSeries;
	std::deque<MessageRange> m_rangesToSkip;
	byte *m_begin;
	size_t m_length;
};

//! _
class CRYPTOPP_DLL TransparentFilter : public MeterFilter
{
public:
	TransparentFilter(BufferedTransformation *attachment=NULL) : MeterFilter(attachment, true) {}
};

//! _
class CRYPTOPP_DLL OpaqueFilter : public MeterFilter
{
public:
	OpaqueFilter(BufferedTransformation *attachment=NULL) : MeterFilter(attachment, false) {}
};

/*! FilterWithBufferedInput divides up the input stream into
	a first block, a number of middle blocks, and a last block.
	First and last blocks are optional, and middle blocks may
	be a stream instead (i.e. blockSize == 1).
*/
class CRYPTOPP_DLL FilterWithBufferedInput : public Filter
{
public:
	FilterWithBufferedInput(BufferedTransformation *attachment);
	//! firstSize and lastSize may be 0, blockSize must be at least 1
	FilterWithBufferedInput(size_t firstSize, size_t blockSize, size_t lastSize, BufferedTransformation *attachment);

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
	{
		return PutMaybeModifiable(const_cast<byte *>(inString), length, messageEnd, blocking, false);
	}
	size_t PutModifiable2(byte *inString, size_t length, int messageEnd, bool blocking)
	{
		return PutMaybeModifiable(inString, length, messageEnd, blocking, true);
	}
	/*! calls ForceNextPut() if hardFlush is true */
	bool IsolatedFlush(bool hardFlush, bool blocking);

	/*! The input buffer may contain more than blockSize bytes if lastSize != 0.
		ForceNextPut() forces a call to NextPut() if this is the case.
	*/
	void ForceNextPut();

protected:
	bool DidFirstPut() {return m_firstInputDone;}

	virtual void InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, size_t &firstSize, size_t &blockSize, size_t &lastSize)
		{InitializeDerived(parameters);}
	virtual void InitializeDerived(const NameValuePairs &parameters) {}
	// FirstPut() is called if (firstSize != 0 and totalLength >= firstSize)
	// or (firstSize == 0 and (totalLength > 0 or a MessageEnd() is received))
	virtual void FirstPut(const byte *inString) =0;
	// NextPut() is called if totalLength >= firstSize+blockSize+lastSize
	virtual void NextPutSingle(const byte *inString) {assert(false);}
	// Same as NextPut() except length can be a multiple of blockSize
	// Either NextPut() or NextPutMultiple() must be overriden
	virtual void NextPutMultiple(const byte *inString, size_t length);
	// Same as NextPutMultiple(), but inString can be modified
	virtual void NextPutModifiable(byte *inString, size_t length)
		{NextPutMultiple(inString, length);}
	// LastPut() is always called
	// if totalLength < firstSize then length == totalLength
	// else if totalLength <= firstSize+lastSize then length == totalLength-firstSize
	// else lastSize <= length < lastSize+blockSize
	virtual void LastPut(const byte *inString, size_t length) =0;
	virtual void FlushDerived() {}

protected:
	size_t PutMaybeModifiable(byte *begin, size_t length, int messageEnd, bool blocking, bool modifiable);
	void NextPutMaybeModifiable(byte *inString, size_t length, bool modifiable)
	{
		if (modifiable) NextPutModifiable(inString, length);
		else NextPutMultiple(inString, length);
	}

	// This function should no longer be used, put this here to cause a compiler error
	// if someone tries to override NextPut().
	virtual int NextPut(const byte *inString, size_t length) {assert(false); return 0;}

	class BlockQueue
	{
	public:
		void ResetQueue(size_t blockSize, size_t maxBlocks);
		byte *GetBlock();
		byte *GetContigousBlocks(size_t &numberOfBytes);
		size_t GetAll(byte *outString);
		void Put(const byte *inString, size_t length);
		size_t CurrentSize() const {return m_size;}
		size_t MaxSize() const {return m_buffer.size();}

	private:
		SecByteBlock m_buffer;
		size_t m_blockSize, m_maxBlocks, m_size;
		byte *m_begin;
	};

	size_t m_firstSize, m_blockSize, m_lastSize;
	bool m_firstInputDone;
	BlockQueue m_queue;
};

//! _
class CRYPTOPP_DLL FilterWithInputQueue : public Filter
{
public:
	FilterWithInputQueue(BufferedTransformation *attachment=NULL) : Filter(attachment) {}

	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
	{
		if (!blocking)
			throw BlockingInputOnly("FilterWithInputQueue");
		
		m_inQueue.Put(inString, length);
		if (messageEnd)
		{
			IsolatedMessageEnd(blocking);
			Output(0, NULL, 0, messageEnd, blocking);
		}
		return 0;
	}

protected:
	virtual bool IsolatedMessageEnd(bool blocking) =0;
	void IsolatedInitialize(const NameValuePairs &parameters) {m_inQueue.Clear();}

	ByteQueue m_inQueue;
};

struct BlockPaddingSchemeDef
{
	enum BlockPaddingScheme {NO_PADDING, ZEROS_PADDING, PKCS_PADDING, ONE_AND_ZEROS_PADDING, DEFAULT_PADDING};
};

//! Filter Wrapper for StreamTransformation, optionally handling padding/unpadding when needed
class CRYPTOPP_DLL StreamTransformationFilter : public FilterWithBufferedInput, public BlockPaddingSchemeDef, private FilterPutSpaceHelper
{
public:
	/*! DEFAULT_PADDING means PKCS_PADDING if c.MandatoryBlockSize() > 1 && c.MinLastBlockSize() == 0 (e.g. ECB or CBC mode),
		otherwise NO_PADDING (OFB, CFB, CTR, CBC-CTS modes).
		See http://www.weidai.com/scan-mirror/csp.html for details of the padding schemes. */
	StreamTransformationFilter(StreamTransformation &c, BufferedTransformation *attachment = NULL, BlockPaddingScheme padding = DEFAULT_PADDING, bool allowAuthenticatedSymmetricCipher = false);

	std::string AlgorithmName() const {return m_cipher.AlgorithmName();}

protected:
	void InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, size_t &firstSize, size_t &blockSize, size_t &lastSize);
	void FirstPut(const byte *inString);
	void NextPutMultiple(const byte *inString, size_t length);
	void NextPutModifiable(byte *inString, size_t length);
	void LastPut(const byte *inString, size_t length);

	static size_t LastBlockSize(StreamTransformation &c, BlockPaddingScheme padding);

	StreamTransformation &m_cipher;
	BlockPaddingScheme m_padding;
	unsigned int m_optimalBufferSize;
};

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
typedef StreamTransformationFilter StreamCipherFilter;
#endif

//! Filter Wrapper for HashTransformation
class CRYPTOPP_DLL HashFilter : public Bufferless<Filter>, private FilterPutSpaceHelper
{
public:
	HashFilter(HashTransformation &hm, BufferedTransformation *attachment = NULL, bool putMessage=false, int truncatedDigestSize=-1, const std::string &messagePutChannel=DEFAULT_CHANNEL, const std::string &hashPutChannel=DEFAULT_CHANNEL);

	std::string AlgorithmName() const {return m_hashModule.AlgorithmName();}
	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);
	byte * CreatePutSpace(size_t &size) {return m_hashModule.CreateUpdateSpace(size);}

private:
	HashTransformation &m_hashModule;
	bool m_putMessage;
	unsigned int m_digestSize;
	byte *m_space;
	std::string m_messagePutChannel, m_hashPutChannel;
};

//! Filter Wrapper for HashTransformation
class CRYPTOPP_DLL HashVerificationFilter : public FilterWithBufferedInput
{
public:
	class HashVerificationFailed : public Exception
	{
	public:
		HashVerificationFailed()
			: Exception(DATA_INTEGRITY_CHECK_FAILED, "HashVerificationFilter: message hash or MAC not valid") {}
	};

	enum Flags {HASH_AT_END=0, HASH_AT_BEGIN=1, PUT_MESSAGE=2, PUT_HASH=4, PUT_RESULT=8, THROW_EXCEPTION=16, DEFAULT_FLAGS = HASH_AT_BEGIN | PUT_RESULT};
	HashVerificationFilter(HashTransformation &hm, BufferedTransformation *attachment = NULL, word32 flags = DEFAULT_FLAGS, int truncatedDigestSize=-1);

	std::string AlgorithmName() const {return m_hashModule.AlgorithmName();}
	bool GetLastResult() const {return m_verified;}

protected:
	void InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, size_t &firstSize, size_t &blockSize, size_t &lastSize);
	void FirstPut(const byte *inString);
	void NextPutMultiple(const byte *inString, size_t length);
	void LastPut(const byte *inString, size_t length);

private:
	friend class AuthenticatedDecryptionFilter;

	HashTransformation &m_hashModule;
	word32 m_flags;
	unsigned int m_digestSize;
	bool m_verified;
	SecByteBlock m_expectedHash;
};

typedef HashVerificationFilter HashVerifier;	// for backwards compatibility

//! Filter wrapper for encrypting with AuthenticatedSymmetricCipher, optionally handling padding/unpadding when needed
/*! Additional authenticated data should be given in channel "AAD". If putAAD is true, AAD will be Put() to the attached BufferedTransformation in channel "AAD". */
class CRYPTOPP_DLL AuthenticatedEncryptionFilter : public StreamTransformationFilter
{
public:
	/*! See StreamTransformationFilter for documentation on BlockPaddingScheme  */
	AuthenticatedEncryptionFilter(AuthenticatedSymmetricCipher &c, BufferedTransformation *attachment = NULL, bool putAAD=false, int truncatedDigestSize=-1, const std::string &macChannel=DEFAULT_CHANNEL, BlockPaddingScheme padding = DEFAULT_PADDING);

	void IsolatedInitialize(const NameValuePairs &parameters);
	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size);
	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking);
	void LastPut(const byte *inString, size_t length);

protected:
	HashFilter m_hf;
};

//! Filter wrapper for decrypting with AuthenticatedSymmetricCipher, optionally handling padding/unpadding when needed
/*! Additional authenticated data should be given in channel "AAD". */
class CRYPTOPP_DLL AuthenticatedDecryptionFilter : public FilterWithBufferedInput, public BlockPaddingSchemeDef
{
public:
	enum Flags {MAC_AT_END=0, MAC_AT_BEGIN=1, THROW_EXCEPTION=16, DEFAULT_FLAGS = THROW_EXCEPTION};

	/*! See StreamTransformationFilter for documentation on BlockPaddingScheme  */
	AuthenticatedDecryptionFilter(AuthenticatedSymmetricCipher &c, BufferedTransformation *attachment = NULL, word32 flags = DEFAULT_FLAGS, int truncatedDigestSize=-1, BlockPaddingScheme padding = DEFAULT_PADDING);

	std::string AlgorithmName() const {return m_hashVerifier.AlgorithmName();}
	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size);
	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking);
	bool GetLastResult() const {return m_hashVerifier.GetLastResult();}

protected:
	void InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, size_t &firstSize, size_t &blockSize, size_t &lastSize);
	void FirstPut(const byte *inString);
	void NextPutMultiple(const byte *inString, size_t length);
	void LastPut(const byte *inString, size_t length);

	HashVerificationFilter m_hashVerifier;
	StreamTransformationFilter m_streamFilter;
};

//! Filter Wrapper for PK_Signer
class CRYPTOPP_DLL SignerFilter : public Unflushable<Filter>
{
public:
	SignerFilter(RandomNumberGenerator &rng, const PK_Signer &signer, BufferedTransformation *attachment = NULL, bool putMessage=false)
		: m_rng(rng), m_signer(signer), m_messageAccumulator(signer.NewSignatureAccumulator(rng)), m_putMessage(putMessage) {Detach(attachment);}

	std::string AlgorithmName() const {return m_signer.AlgorithmName();}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

private:
	RandomNumberGenerator &m_rng;
	const PK_Signer &m_signer;
	member_ptr<PK_MessageAccumulator> m_messageAccumulator;
	bool m_putMessage;
	SecByteBlock m_buf;
};

//! Filter Wrapper for PK_Verifier
class CRYPTOPP_DLL SignatureVerificationFilter : public FilterWithBufferedInput
{
public:
	class SignatureVerificationFailed : public Exception
	{
	public:
		SignatureVerificationFailed()
			: Exception(DATA_INTEGRITY_CHECK_FAILED, "VerifierFilter: digital signature not valid") {}
	};

	enum Flags {SIGNATURE_AT_END=0, SIGNATURE_AT_BEGIN=1, PUT_MESSAGE=2, PUT_SIGNATURE=4, PUT_RESULT=8, THROW_EXCEPTION=16, DEFAULT_FLAGS = SIGNATURE_AT_BEGIN | PUT_RESULT};
	SignatureVerificationFilter(const PK_Verifier &verifier, BufferedTransformation *attachment = NULL, word32 flags = DEFAULT_FLAGS);

	std::string AlgorithmName() const {return m_verifier.AlgorithmName();}

	bool GetLastResult() const {return m_verified;}

protected:
	void InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, size_t &firstSize, size_t &blockSize, size_t &lastSize);
	void FirstPut(const byte *inString);
	void NextPutMultiple(const byte *inString, size_t length);
	void LastPut(const byte *inString, size_t length);

private:
	const PK_Verifier &m_verifier;
	member_ptr<PK_MessageAccumulator> m_messageAccumulator;
	word32 m_flags;
	SecByteBlock m_signature;
	bool m_verified;
};

typedef SignatureVerificationFilter VerifierFilter;	// for backwards compatibility

//! Redirect input to another BufferedTransformation without owning it
class CRYPTOPP_DLL Redirector : public CustomSignalPropagation<Sink>
{
public:
	enum Behavior
	{
		DATA_ONLY = 0x00,
		PASS_SIGNALS = 0x01,
		PASS_WAIT_OBJECTS = 0x02,
		PASS_EVERYTHING = PASS_SIGNALS | PASS_WAIT_OBJECTS
	};

	Redirector() : m_target(NULL), m_behavior(PASS_EVERYTHING) {}
	Redirector(BufferedTransformation &target, Behavior behavior=PASS_EVERYTHING)
		: m_target(&target), m_behavior(behavior) {}

	void Redirect(BufferedTransformation &target) {m_target = &target;}
	void StopRedirection() {m_target = NULL;}

	Behavior GetBehavior() {return (Behavior) m_behavior;}
	void SetBehavior(Behavior behavior) {m_behavior=behavior;}
	bool GetPassSignals() const {return (m_behavior & PASS_SIGNALS) != 0;}
	void SetPassSignals(bool pass) { if (pass) m_behavior |= PASS_SIGNALS; else m_behavior &= ~(word32) PASS_SIGNALS; }
	bool GetPassWaitObjects() const {return (m_behavior & PASS_WAIT_OBJECTS) != 0;}
	void SetPassWaitObjects(bool pass) { if (pass) m_behavior |= PASS_WAIT_OBJECTS; else m_behavior &= ~(word32) PASS_WAIT_OBJECTS; }

	bool CanModifyInput() const
		{return m_target ? m_target->CanModifyInput() : false;}

	void Initialize(const NameValuePairs &parameters, int propagation);
	byte * CreatePutSpace(size_t &size)
		{return m_target ? m_target->CreatePutSpace(size) : (byte *)(size=0, NULL);}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_target ? m_target->Put2(begin, length, GetPassSignals() ? messageEnd : 0, blocking) : 0;}
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true)
		{return m_target && GetPassSignals() ? m_target->Flush(hardFlush, propagation, blocking) : false;}
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true)
		{return m_target && GetPassSignals() ? m_target->MessageSeriesEnd(propagation, blocking) : false;}

	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size)
		{return m_target ? m_target->ChannelCreatePutSpace(channel, size) : (byte *)(size=0, NULL);}
	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_target ? m_target->ChannelPut2(channel, begin, length, GetPassSignals() ? messageEnd : 0, blocking) : 0;}
	size_t ChannelPutModifiable2(const std::string &channel, byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_target ? m_target->ChannelPutModifiable2(channel, begin, length, GetPassSignals() ? messageEnd : 0, blocking) : 0;}
	bool ChannelFlush(const std::string &channel, bool completeFlush, int propagation=-1, bool blocking=true)
		{return m_target && GetPassSignals() ? m_target->ChannelFlush(channel, completeFlush, propagation, blocking) : false;}
	bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true)
		{return m_target && GetPassSignals() ? m_target->ChannelMessageSeriesEnd(channel, propagation, blocking) : false;}

	unsigned int GetMaxWaitObjectCount() const
		{ return m_target && GetPassWaitObjects() ? m_target->GetMaxWaitObjectCount() : 0; }
	void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
		{ if (m_target && GetPassWaitObjects()) m_target->GetWaitObjects(container, callStack); }

private:
	BufferedTransformation *m_target;
	word32 m_behavior;
};

// Used By ProxyFilter
class CRYPTOPP_DLL OutputProxy : public CustomSignalPropagation<Sink>
{
public:
	OutputProxy(BufferedTransformation &owner, bool passSignal) : m_owner(owner), m_passSignal(passSignal) {}

	bool GetPassSignal() const {return m_passSignal;}
	void SetPassSignal(bool passSignal) {m_passSignal = passSignal;}

	byte * CreatePutSpace(size_t &size)
		{return m_owner.AttachedTransformation()->CreatePutSpace(size);}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->Put2(begin, length, m_passSignal ? messageEnd : 0, blocking);}
	size_t PutModifiable2(byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->PutModifiable2(begin, length, m_passSignal ? messageEnd : 0, blocking);}
	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1)
		{if (m_passSignal) m_owner.AttachedTransformation()->Initialize(parameters, propagation);}
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->Flush(hardFlush, propagation, blocking) : false;}
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->MessageSeriesEnd(propagation, blocking) : false;}

	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size)
		{return m_owner.AttachedTransformation()->ChannelCreatePutSpace(channel, size);}
	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->ChannelPut2(channel, begin, length, m_passSignal ? messageEnd : 0, blocking);}
	size_t ChannelPutModifiable2(const std::string &channel, byte *begin, size_t length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->ChannelPutModifiable2(channel, begin, length, m_passSignal ? messageEnd : 0, blocking);}
	bool ChannelFlush(const std::string &channel, bool completeFlush, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->ChannelFlush(channel, completeFlush, propagation, blocking) : false;}
	bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->ChannelMessageSeriesEnd(channel, propagation, blocking) : false;}

private:
	BufferedTransformation &m_owner;
	bool m_passSignal;
};

//! Base class for Filter classes that are proxies for a chain of other filters.
class CRYPTOPP_DLL ProxyFilter : public FilterWithBufferedInput
{
public:
	ProxyFilter(BufferedTransformation *filter, size_t firstSize, size_t lastSize, BufferedTransformation *attachment);

	bool IsolatedFlush(bool hardFlush, bool blocking);

	void SetFilter(Filter *filter);
	void NextPutMultiple(const byte *s, size_t len);
	void NextPutModifiable(byte *inString, size_t length);

protected:
	member_ptr<BufferedTransformation> m_filter;
};

//! simple proxy filter that doesn't modify the underlying filter's input or output
class CRYPTOPP_DLL SimpleProxyFilter : public ProxyFilter
{
public:
	SimpleProxyFilter(BufferedTransformation *filter, BufferedTransformation *attachment)
		: ProxyFilter(filter, 0, 0, attachment) {}

	void FirstPut(const byte *) {}
	void LastPut(const byte *, size_t) {m_filter->MessageEnd();}
};

//! proxy for the filter created by PK_Encryptor::CreateEncryptionFilter
/*! This class is here just to provide symmetry with VerifierFilter. */
class CRYPTOPP_DLL PK_EncryptorFilter : public SimpleProxyFilter
{
public:
	PK_EncryptorFilter(RandomNumberGenerator &rng, const PK_Encryptor &encryptor, BufferedTransformation *attachment = NULL)
		: SimpleProxyFilter(encryptor.CreateEncryptionFilter(rng), attachment) {}
};

//! proxy for the filter created by PK_Decryptor::CreateDecryptionFilter
/*! This class is here just to provide symmetry with SignerFilter. */
class CRYPTOPP_DLL PK_DecryptorFilter : public SimpleProxyFilter
{
public:
	PK_DecryptorFilter(RandomNumberGenerator &rng, const PK_Decryptor &decryptor, BufferedTransformation *attachment = NULL)
		: SimpleProxyFilter(decryptor.CreateDecryptionFilter(rng), attachment) {}
};

//! Append input to a string object
template <class T>
class StringSinkTemplate : public Bufferless<Sink>
{
public:
	// VC60 workaround: no T::char_type
	typedef typename T::traits_type::char_type char_type;

	StringSinkTemplate(T &output)
		: m_output(&output) {assert(sizeof(output[0])==1);}

	void IsolatedInitialize(const NameValuePairs &parameters)
		{if (!parameters.GetValue("OutputStringPointer", m_output)) throw InvalidArgument("StringSink: OutputStringPointer not specified");}

	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
	{
		if (length > 0)
		{
			typename T::size_type size = m_output->size();
			if (length < size && size + length > m_output->capacity())
				m_output->reserve(2*size);
		m_output->append((const char_type *)begin, (const char_type *)begin+length);
		}
		return 0;
	}

private:	
	T *m_output;
};

//! Append input to an std::string
CRYPTOPP_DLL_TEMPLATE_CLASS StringSinkTemplate<std::string>;
typedef StringSinkTemplate<std::string> StringSink;

//! incorporates input into RNG as additional entropy
class RandomNumberSink : public Bufferless<Sink>
{
public:
	RandomNumberSink()
		: m_rng(NULL) {}

	RandomNumberSink(RandomNumberGenerator &rng)
		: m_rng(&rng) {}

	void IsolatedInitialize(const NameValuePairs &parameters);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

private:
	RandomNumberGenerator *m_rng;
};

//! Copy input to a memory buffer
class CRYPTOPP_DLL ArraySink : public Bufferless<Sink>
{
public:
	ArraySink(const NameValuePairs &parameters = g_nullNameValuePairs) {IsolatedInitialize(parameters);}
	ArraySink(byte *buf, size_t size) : m_buf(buf), m_size(size), m_total(0) {}

	size_t AvailableSize() {return SaturatingSubtract(m_size, m_total);}
	lword TotalPutLength() {return m_total;}

	void IsolatedInitialize(const NameValuePairs &parameters);
	byte * CreatePutSpace(size_t &size);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

protected:
	byte *m_buf;
	size_t m_size;
	lword m_total;
};

//! Xor input to a memory buffer
class CRYPTOPP_DLL ArrayXorSink : public ArraySink
{
public:
	ArrayXorSink(byte *buf, size_t size)
		: ArraySink(buf, size) {}

	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);
	byte * CreatePutSpace(size_t &size) {return BufferedTransformation::CreatePutSpace(size);}
};

//! string-based implementation of Store interface
class StringStore : public Store
{
public:
	StringStore(const char *string = NULL)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
	StringStore(const byte *string, size_t length)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string, length)));}
	template <class T> StringStore(const T &string)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}

	CRYPTOPP_DLL size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	CRYPTOPP_DLL size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

private:
	CRYPTOPP_DLL void StoreInitialize(const NameValuePairs &parameters);

	const byte *m_store;
	size_t m_length, m_count;
};

//! RNG-based implementation of Source interface
class CRYPTOPP_DLL RandomNumberStore : public Store
{
public:
	RandomNumberStore()
		: m_rng(NULL), m_length(0), m_count(0) {}

	RandomNumberStore(RandomNumberGenerator &rng, lword length)
		: m_rng(&rng), m_length(length), m_count(0) {}

	bool AnyRetrievable() const {return MaxRetrievable() != 0;}
	lword MaxRetrievable() const {return m_length-m_count;}

	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const
	{
		throw NotImplemented("RandomNumberStore: CopyRangeTo2() is not supported by this store");
	}

private:
	void StoreInitialize(const NameValuePairs &parameters);

	RandomNumberGenerator *m_rng;
	lword m_length, m_count;
};

//! empty store
class CRYPTOPP_DLL NullStore : public Store
{
public:
	NullStore(lword size = ULONG_MAX) : m_size(size) {}
	void StoreInitialize(const NameValuePairs &parameters) {}
	lword MaxRetrievable() const {return m_size;}
	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

private:
	lword m_size;
};

//! A Filter that pumps data into its attachment as input
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Source : public InputRejecting<Filter>
{
public:
	Source(BufferedTransformation *attachment = NULL)
		{Source::Detach(attachment);}

	lword Pump(lword pumpMax=size_t(0)-1)
		{Pump2(pumpMax); return pumpMax;}
	unsigned int PumpMessages(unsigned int count=UINT_MAX)
		{PumpMessages2(count); return count;}
	void PumpAll()
		{PumpAll2();}
	virtual size_t Pump2(lword &byteCount, bool blocking=true) =0;
	virtual size_t PumpMessages2(unsigned int &messageCount, bool blocking=true) =0;
	virtual size_t PumpAll2(bool blocking=true);
	virtual bool SourceExhausted() const =0;

protected:
	void SourceInitialize(bool pumpAll, const NameValuePairs &parameters)
	{
		IsolatedInitialize(parameters);
		if (pumpAll)
			PumpAll();
	}
};

//! Turn a Store into a Source
template <class T>
class SourceTemplate : public Source
{
public:
	SourceTemplate<T>(BufferedTransformation *attachment)
		: Source(attachment) {}
	void IsolatedInitialize(const NameValuePairs &parameters)
		{m_store.IsolatedInitialize(parameters);}
	size_t Pump2(lword &byteCount, bool blocking=true)
		{return m_store.TransferTo2(*AttachedTransformation(), byteCount, DEFAULT_CHANNEL, blocking);}
	size_t PumpMessages2(unsigned int &messageCount, bool blocking=true)
		{return m_store.TransferMessagesTo2(*AttachedTransformation(), messageCount, DEFAULT_CHANNEL, blocking);}
	size_t PumpAll2(bool blocking=true)
		{return m_store.TransferAllTo2(*AttachedTransformation(), DEFAULT_CHANNEL, blocking);}
	bool SourceExhausted() const
		{return !m_store.AnyRetrievable() && !m_store.AnyMessages();}
	void SetAutoSignalPropagation(int propagation)
		{m_store.SetAutoSignalPropagation(propagation);}
	int GetAutoSignalPropagation() const
		{return m_store.GetAutoSignalPropagation();}

protected:
	T m_store;
};

//! string-based implementation of Source interface
class CRYPTOPP_DLL StringSource : public SourceTemplate<StringStore>
{
public:
	StringSource(BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {}
	//! zero terminated string as source
	StringSource(const char *string, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
	//! binary byte array as source
	StringSource(const byte *string, size_t length, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string, length)));}
	//! std::string as source
	StringSource(const std::string &string, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
};

//! use the third constructor for an array source
typedef StringSource ArraySource;

//! RNG-based implementation of Source interface
class CRYPTOPP_DLL RandomNumberSource : public SourceTemplate<RandomNumberStore>
{
public:
	RandomNumberSource(RandomNumberGenerator &rng, int length, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<RandomNumberStore>(attachment) 
		{SourceInitialize(pumpAll, MakeParameters("RandomNumberGeneratorPointer", &rng)("RandomNumberStoreSize", length));}
};

NAMESPACE_END

#endif
