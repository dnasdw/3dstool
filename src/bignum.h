#ifndef BIGNUM_H_
#define BIGNUM_H_

#include <openssl/bn.h>

class CBigNum
{
public:
	CBigNum();
	CBigNum(int a_nInteger);
	CBigNum(const char* a_pString);
	CBigNum(const CBigNum& a_BigNum);
	~CBigNum();
	void GetBytes(unsigned char* a_pBytes, int a_nBytesSize) const;
	CBigNum Crol(int a_nBits, int a_nBitsMax) const;
	CBigNum operator+(const CBigNum& a_BigNum) const;
	CBigNum operator-(const CBigNum& a_BigNum) const;
	CBigNum operator<<(int a_nBits) const;
	CBigNum operator>>(int a_nBits) const;
	bool operator==(const CBigNum& a_BigNum) const;
	CBigNum operator&(const CBigNum& a_BigNum) const;
	CBigNum operator|(const CBigNum& a_BigNum) const;
	CBigNum operator^(const CBigNum& a_BigNum) const;
	CBigNum& operator=(int a_nInteger);
	CBigNum& operator=(const char* a_pString);
	CBigNum& operator=(const CBigNum& a_BigNum);
	CBigNum& operator+=(const CBigNum& a_BigNum);
	CBigNum& operator-=(const CBigNum& a_BigNum);
	CBigNum& operator<<=(int a_nBits);
	CBigNum& operator>>=(int a_nBits);
	CBigNum& operator&=(const CBigNum& a_BigNum);
	CBigNum& operator|=(const CBigNum& a_BigNum);
	CBigNum& operator^=(const CBigNum& a_BigNum);
private:
	void constructor();
	void destructor();
	BIGNUM* m_pValue;
	BN_CTX* m_pCtx;
};

#endif	// BIGNUM_H_
