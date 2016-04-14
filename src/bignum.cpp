#include "bignum.h"
#include "utility.h"

CBigNum::CBigNum()
	: m_pValue(nullptr)
	, m_pCtx(nullptr)
{
	constructor();
}

CBigNum::CBigNum(int a_nInteger)
	: m_pValue(nullptr)
	, m_pCtx(nullptr)
{
	constructor();
	unsigned int uInteger = static_cast<unsigned int>(a_nInteger);
	BN_set_word(m_pValue, uInteger);
}

CBigNum::CBigNum(const char* a_pString)
	: m_pValue(nullptr)
	, m_pCtx(nullptr)
{
	constructor();
	BN_hex2bn(&m_pValue, a_pString);
}

CBigNum::CBigNum(const CBigNum& a_BigNum)
	: m_pValue(nullptr)
	, m_pCtx(nullptr)
{
	constructor();
	BN_copy(m_pValue, a_BigNum.m_pValue);
}

CBigNum::~CBigNum()
{
	destructor();
}

void CBigNum::GetBytes(unsigned char* a_pBytes, int a_nBytesSize) const
{
	int nBytesSizeDelta = a_nBytesSize - BN_num_bytes(m_pValue);
	CBigNum bigNum;
	if (nBytesSizeDelta >= 0)
	{
		bigNum = *this;
	}
	else
	{
		BN_set_bit(bigNum.m_pValue, a_nBytesSize * 8);
		BN_sub_word(bigNum.m_pValue, 1);
		bigNum &= *this;
		nBytesSizeDelta = a_nBytesSize - BN_num_bytes(bigNum.m_pValue);
	}
	memset(a_pBytes, 0, nBytesSizeDelta);
	BN_bn2bin(bigNum.m_pValue, a_pBytes + nBytesSizeDelta);
}

CBigNum CBigNum::Crol(int a_nBits, int a_nBitsMax) const
{
	CBigNum bigNum;
	if (a_nBitsMax == 0)
	{
		return bigNum;
	}
	a_nBits %= a_nBitsMax;
	CBigNum leftMask;
	CBigNum rightMask;
	for (int i = 0; i < a_nBitsMax; i++)
	{
		if (i < a_nBits)
		{
			BN_set_bit(rightMask.m_pValue, i);
		}
		else
		{
			BN_set_bit(leftMask.m_pValue, i);
		}
	}
	bigNum = (*this << a_nBits & leftMask) | (*this >> (a_nBitsMax - a_nBits) & rightMask);
	return bigNum;
}

CBigNum CBigNum::operator+(const CBigNum& a_BigNum) const
{
	CBigNum bigNum;
	BN_add(bigNum.m_pValue, m_pValue, a_BigNum.m_pValue);
	return bigNum;
}

CBigNum CBigNum::operator-(const CBigNum& a_BigNum) const
{
	CBigNum bigNum;
	BN_sub(bigNum.m_pValue, m_pValue, a_BigNum.m_pValue);
	return bigNum;
}

CBigNum CBigNum::operator<<(int a_nBits) const
{
	CBigNum bigNum;
	BN_lshift(bigNum.m_pValue, m_pValue, a_nBits);
	return bigNum;
}

CBigNum CBigNum::operator>>(int a_nBits) const
{
	CBigNum bigNum;
	BN_rshift(bigNum.m_pValue, m_pValue, a_nBits);
	return bigNum;
}

bool CBigNum::operator==(const CBigNum& a_BigNum) const
{
	return BN_cmp(m_pValue, a_BigNum.m_pValue) == 0;
}

CBigNum CBigNum::operator&(const CBigNum& a_BigNum) const
{
	int nBits = min<int>(BN_num_bits(m_pValue), BN_num_bits(a_BigNum.m_pValue));
	CBigNum bigNum;
	for (int i = 0; i < nBits; i++)
	{
		if (BN_is_bit_set(m_pValue, i) == 1 && BN_is_bit_set(a_BigNum.m_pValue, i) == 1)
		{
			BN_set_bit(bigNum.m_pValue, i);
		}
	}
	return bigNum;
}

CBigNum CBigNum::operator|(const CBigNum& a_BigNum) const
{
	int nBits = max<int>(BN_num_bits(m_pValue), BN_num_bits(a_BigNum.m_pValue));
	CBigNum bigNum;
	for (int i = 0; i < nBits; i++)
	{
		if (BN_is_bit_set(m_pValue, i) == 1 || BN_is_bit_set(a_BigNum.m_pValue, i) == 1)
		{
			BN_set_bit(bigNum.m_pValue, i);
		}
	}
	return bigNum;
}

CBigNum CBigNum::operator^(const CBigNum& a_BigNum) const
{
	int nBits = max<int>(BN_num_bits(m_pValue), BN_num_bits(a_BigNum.m_pValue));
	CBigNum bigNum;
	for (int i = 0; i < nBits; i++)
	{
		if (BN_is_bit_set(m_pValue, i) != BN_is_bit_set(a_BigNum.m_pValue, i))
		{
			BN_set_bit(bigNum.m_pValue, i);
		}
	}
	return bigNum;
}

CBigNum& CBigNum::operator=(int a_nInteger)
{
	unsigned int uInteger = static_cast<unsigned int>(a_nInteger);
	BN_set_word(m_pValue, uInteger);
	return *this;
}

CBigNum& CBigNum::operator=(const char* a_pString)
{
	BN_hex2bn(&m_pValue, a_pString);
	return *this;
}

CBigNum& CBigNum::operator=(const CBigNum& a_BigNum)
{
	BN_copy(m_pValue, a_BigNum.m_pValue);
	return *this;
}

CBigNum& CBigNum::operator+=(const CBigNum& a_BigNum)
{
	*this = *this + a_BigNum;
	return *this;
}

CBigNum& CBigNum::operator-=(const CBigNum& a_BigNum)
{
	*this = *this - a_BigNum;
	return *this;
}

CBigNum& CBigNum::operator<<=(int a_nBits)
{
	*this = *this << a_nBits;
	return *this;
}

CBigNum& CBigNum::operator>>=(int a_nBits)
{
	*this = *this >> a_nBits;
	return *this;
}

CBigNum& CBigNum::operator&=(const CBigNum& a_BigNum)
{
	*this = *this & a_BigNum;
	return *this;
}

CBigNum& CBigNum::operator|=(const CBigNum& a_BigNum)
{
	*this = *this | a_BigNum;
	return *this;
}

CBigNum& CBigNum::operator^=(const CBigNum& a_BigNum)
{
	*this = *this ^ a_BigNum;
	return *this;
}

void CBigNum::constructor()
{
	m_pValue = BN_new();
	m_pCtx = BN_CTX_new();
	if (m_pCtx != nullptr)
	{
		BN_CTX_start(m_pCtx);
	}
}

void CBigNum::destructor()
{
	if (m_pCtx != nullptr)
	{
		BN_CTX_end(m_pCtx);
		BN_CTX_free(m_pCtx);
	}
	if (m_pValue != nullptr)
	{
		BN_free(m_pValue);
	}
}
