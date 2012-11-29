#ifndef CRYPTOPP_INTEGER_H
#define CRYPTOPP_INTEGER_H

/** \file */

#include "cryptlib.h"
#include "secblock.h"

#include <iosfwd>
#include <algorithm>

NAMESPACE_BEGIN(CryptoPP)

struct InitializeInteger	// used to initialize static variables
{
	InitializeInteger();
};

typedef SecBlock<word, AllocatorWithCleanup<word, CRYPTOPP_BOOL_X86> > IntegerSecBlock;

//! multiple precision integer and basic arithmetics
/*! This class can represent positive and negative integers
	with absolute value less than (256**sizeof(word)) ** (256**sizeof(int)).
	\nosubgrouping
*/
class CRYPTOPP_DLL Integer : private InitializeInteger, public ASN1Object
{
public:
	//! \name ENUMS, EXCEPTIONS, and TYPEDEFS
	//@{
		//! division by zero exception
		class DivideByZero : public Exception
		{
		public:
			DivideByZero() : Exception(OTHER_ERROR, "Integer: division by zero") {}
		};

		//!
		class RandomNumberNotFound : public Exception
		{
		public:
			RandomNumberNotFound() : Exception(OTHER_ERROR, "Integer: no integer satisfies the given parameters") {}
		};

		//!
		enum Sign {POSITIVE=0, NEGATIVE=1};

		//!
		enum Signedness {
		//!
			UNSIGNED,
		//!
			SIGNED};

		//!
		enum RandomNumberType {
		//!
			ANY,
		//!
			PRIME};
	//@}

	//! \name CREATORS
	//@{
		//! creates the zero integer
		Integer();

		//! copy constructor
		Integer(const Integer& t);

		//! convert from signed long
		Integer(signed long value);

		//! convert from lword
		Integer(Sign s, lword value);

		//! convert from two words
		Integer(Sign s, word highWord, word lowWord);

		//! convert from string
		/*! str can be in base 2, 8, 10, or 16.  Base is determined by a
			case insensitive suffix of 'h', 'o', or 'b'.  No suffix means base 10.
		*/
		explicit Integer(const char *str);
		explicit Integer(const wchar_t *str);

		//! convert from big-endian byte array
		Integer(const byte *encodedInteger, size_t byteCount, Signedness s=UNSIGNED);

		//! convert from big-endian form stored in a BufferedTransformation
		Integer(BufferedTransformation &bt, size_t byteCount, Signedness s=UNSIGNED);

		//! convert from BER encoded byte array stored in a BufferedTransformation object
		explicit Integer(BufferedTransformation &bt);

		//! create a random integer
		/*! The random integer created is uniformly distributed over [0, 2**bitcount). */
		Integer(RandomNumberGenerator &rng, size_t bitcount);

		//! avoid calling constructors for these frequently used integers
		static const Integer & CRYPTOPP_API Zero();
		//! avoid calling constructors for these frequently used integers
		static const Integer & CRYPTOPP_API One();
		//! avoid calling constructors for these frequently used integers
		static const Integer & CRYPTOPP_API Two();

		//! create a random integer of special type
		/*! Ideally, the random integer created should be uniformly distributed
			over {x | min <= x <= max and x is of rnType and x % mod == equiv}.
			However the actual distribution may not be uniform because sequential
			search is used to find an appropriate number from a random starting
			point.
			May return (with very small probability) a pseudoprime when a prime
			is requested and max > lastSmallPrime*lastSmallPrime (lastSmallPrime
			is declared in nbtheory.h).
			\throw RandomNumberNotFound if the set is empty.
		*/
		Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType=ANY, const Integer &equiv=Zero(), const Integer &mod=One());

		//! return the integer 2**e
		static Integer CRYPTOPP_API Power2(size_t e);
	//@}

	//! \name ENCODE/DECODE
	//@{
		//! minimum number of bytes to encode this integer
		/*! MinEncodedSize of 0 is 1 */
		size_t MinEncodedSize(Signedness=UNSIGNED) const;
		//! encode in big-endian format
		/*! unsigned means encode absolute value, signed means encode two's complement if negative.
			if outputLen < MinEncodedSize, the most significant bytes will be dropped
			if outputLen > MinEncodedSize, the most significant bytes will be padded
		*/
		void Encode(byte *output, size_t outputLen, Signedness=UNSIGNED) const;
		//!
		void Encode(BufferedTransformation &bt, size_t outputLen, Signedness=UNSIGNED) const;

		//! encode using Distinguished Encoding Rules, put result into a BufferedTransformation object
		void DEREncode(BufferedTransformation &bt) const;

		//! encode absolute value as big-endian octet string
		void DEREncodeAsOctetString(BufferedTransformation &bt, size_t length) const;

		//! encode absolute value in OpenPGP format, return length of output
		size_t OpenPGPEncode(byte *output, size_t bufferSize) const;
		//! encode absolute value in OpenPGP format, put result into a BufferedTransformation object
		size_t OpenPGPEncode(BufferedTransformation &bt) const;

		//!
		void Decode(const byte *input, size_t inputLen, Signedness=UNSIGNED);
		//! 
		//* Precondition: bt.MaxRetrievable() >= inputLen
		void Decode(BufferedTransformation &bt, size_t inputLen, Signedness=UNSIGNED);

		//!
		void BERDecode(const byte *input, size_t inputLen);
		//!
		void BERDecode(BufferedTransformation &bt);

		//! decode nonnegative value as big-endian octet string
		void BERDecodeAsOctetString(BufferedTransformation &bt, size_t length);

		class OpenPGPDecodeErr : public Exception
		{
		public: 
			OpenPGPDecodeErr() : Exception(INVALID_DATA_FORMAT, "OpenPGP decode error") {}
		};

		//!
		void OpenPGPDecode(const byte *input, size_t inputLen);
		//!
		void OpenPGPDecode(BufferedTransformation &bt);
	//@}

	//! \name ACCESSORS
	//@{
		//! return true if *this can be represented as a signed long
		bool IsConvertableToLong() const;
		//! return equivalent signed long if possible, otherwise undefined
		signed long ConvertToLong() const;

		//! number of significant bits = floor(log2(abs(*this))) + 1
		unsigned int BitCount() const;
		//! number of significant bytes = ceiling(BitCount()/8)
		unsigned int ByteCount() const;
		//! number of significant words = ceiling(ByteCount()/sizeof(word))
		unsigned int WordCount() const;

		//! return the i-th bit, i=0 being the least significant bit
		bool GetBit(size_t i) const;
		//! return the i-th byte
		byte GetByte(size_t i) const;
		//! return n lowest bits of *this >> i
		lword GetBits(size_t i, size_t n) const;

		//!
		bool IsZero() const {return !*this;}
		//!
		bool NotZero() const {return !IsZero();}
		//!
		bool IsNegative() const {return sign == NEGATIVE;}
		//!
		bool NotNegative() const {return !IsNegative();}
		//!
		bool IsPositive() const {return NotNegative() && NotZero();}
		//!
		bool NotPositive() const {return !IsPositive();}
		//!
		bool IsEven() const {return GetBit(0) == 0;}
		//!
		bool IsOdd() const	{return GetBit(0) == 1;}
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		Integer&  operator=(const Integer& t);

		//!
		Integer&  operator+=(const Integer& t);
		//!
		Integer&  operator-=(const Integer& t);
		//!
		Integer&  operator*=(const Integer& t)	{return *this = Times(t);}
		//!
		Integer&  operator/=(const Integer& t)	{return *this = DividedBy(t);}
		//!
		Integer&  operator%=(const Integer& t)	{return *this = Modulo(t);}
		//!
		Integer&  operator/=(word t)  {return *this = DividedBy(t);}
		//!
		Integer&  operator%=(word t)  {return *this = Integer(POSITIVE, 0, Modulo(t));}

		//!
		Integer&  operator<<=(size_t);
		//!
		Integer&  operator>>=(size_t);

		//!
		void Randomize(RandomNumberGenerator &rng, size_t bitcount);
		//!
		void Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max);
		//! set this Integer to a random element of {x | min <= x <= max and x is of rnType and x % mod == equiv}
		/*! returns false if the set is empty */
		bool Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv=Zero(), const Integer &mod=One());

		bool GenerateRandomNoThrow(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs);
		void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs)
		{
			if (!GenerateRandomNoThrow(rng, params))
				throw RandomNumberNotFound();
		}

		//! set the n-th bit to value
		void SetBit(size_t n, bool value=1);
		//! set the n-th byte to value
		void SetByte(size_t n, byte value);

		//!
		void Negate();
		//!
		void SetPositive() {sign = POSITIVE;}
		//!
		void SetNegative() {if (!!(*this)) sign = NEGATIVE;}

		//!
		void swap(Integer &a);
	//@}

	//! \name UNARY OPERATORS
	//@{
		//!
		bool		operator!() const;
		//!
		Integer 	operator+() const {return *this;}
		//!
		Integer 	operator-() const;
		//!
		Integer&	operator++();
		//!
		Integer&	operator--();
		//!
		Integer 	operator++(int) {Integer temp = *this; ++*this; return temp;}
		//!
		Integer 	operator--(int) {Integer temp = *this; --*this; return temp;}
	//@}

	//! \name BINARY OPERATORS
	//@{
		//! signed comparison
		/*! \retval -1 if *this < a
			\retval  0 if *this = a
			\retval  1 if *this > a
		*/
		int Compare(const Integer& a) const;

		//!
		Integer Plus(const Integer &b) const;
		//!
		Integer Minus(const Integer &b) const;
		//!
		Integer Times(const Integer &b) const;
		//!
		Integer DividedBy(const Integer &b) const;
		//!
		Integer Modulo(const Integer &b) const;
		//!
		Integer DividedBy(word b) const;
		//!
		word Modulo(word b) const;

		//!
		Integer operator>>(size_t n) const	{return Integer(*this)>>=n;}
		//!
		Integer operator<<(size_t n) const	{return Integer(*this)<<=n;}
	//@}

	//! \name OTHER ARITHMETIC FUNCTIONS
	//@{
		//!
		Integer AbsoluteValue() const;
		//!
		Integer Doubled() const {return Plus(*this);}
		//!
		Integer Squared() const {return Times(*this);}
		//! extract square root, if negative return 0, else return floor of square root
		Integer SquareRoot() const;
		//! return whether this integer is a perfect square
		bool IsSquare() const;

		//! is 1 or -1
		bool IsUnit() const;
		//! return inverse if 1 or -1, otherwise return 0
		Integer MultiplicativeInverse() const;

		//! modular multiplication
		CRYPTOPP_DLL friend Integer CRYPTOPP_API a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m);
		//! modular exponentiation
		CRYPTOPP_DLL friend Integer CRYPTOPP_API a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m);

		//! calculate r and q such that (a == d*q + r) && (0 <= r < abs(d))
		static void CRYPTOPP_API Divide(Integer &r, Integer &q, const Integer &a, const Integer &d);
		//! use a faster division algorithm when divisor is short
		static void CRYPTOPP_API Divide(word &r, Integer &q, const Integer &a, word d);

		//! returns same result as Divide(r, q, a, Power2(n)), but faster
		static void CRYPTOPP_API DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n);

		//! greatest common divisor
		static Integer CRYPTOPP_API Gcd(const Integer &a, const Integer &n);
		//! calculate multiplicative inverse of *this mod n
		Integer InverseMod(const Integer &n) const;
		//!
		word InverseMod(word n) const;
	//@}

	//! \name INPUT/OUTPUT
	//@{
		//!
		friend CRYPTOPP_DLL std::istream& CRYPTOPP_API operator>>(std::istream& in, Integer &a);
		//!
		friend CRYPTOPP_DLL std::ostream& CRYPTOPP_API operator<<(std::ostream& out, const Integer &a);
	//@}

private:
	friend class ModularArithmetic;
	friend class MontgomeryRepresentation;
	friend class HalfMontgomeryRepresentation;

	Integer(word value, size_t length);

	int PositiveCompare(const Integer &t) const;
	friend void PositiveAdd(Integer &sum, const Integer &a, const Integer &b);
	friend void PositiveSubtract(Integer &diff, const Integer &a, const Integer &b);
	friend void PositiveMultiply(Integer &product, const Integer &a, const Integer &b);
	friend void PositiveDivide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor);

	IntegerSecBlock reg;
	Sign sign;
};

//!
inline bool operator==(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)==0;}
//!
inline bool operator!=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)!=0;}
//!
inline bool operator> (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)> 0;}
//!
inline bool operator>=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)>=0;}
//!
inline bool operator< (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)< 0;}
//!
inline bool operator<=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)<=0;}
//!
inline CryptoPP::Integer operator+(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Plus(b);}
//!
inline CryptoPP::Integer operator-(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Minus(b);}
//!
inline CryptoPP::Integer operator*(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Times(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.DividedBy(b);}
//!
inline CryptoPP::Integer operator%(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Modulo(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, CryptoPP::word b) {return a.DividedBy(b);}
//!
inline CryptoPP::word    operator%(const CryptoPP::Integer &a, CryptoPP::word b) {return a.Modulo(b);}

NAMESPACE_END

#ifndef __BORLANDC__
NAMESPACE_BEGIN(std)
inline void swap(CryptoPP::Integer &a, CryptoPP::Integer &b)
{
	a.swap(b);
}
NAMESPACE_END
#endif

#endif
