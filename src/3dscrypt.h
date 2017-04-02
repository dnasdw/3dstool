#ifndef _3DSCRYPT_H_
#define _3DSCRYPT_H_

#include "utility.h"
#include "bignum.h"

void FEncryptAesCtrCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nSrcOffset, n64 a_nSize);

bool FEncryptXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const string& a_sXorFileName, n64 a_nOffset, n64 a_nSize);

bool FEncryptAesCtrFile(const char* a_pDataFileName, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset);

bool FEncryptXorFile(const string& a_sDataFileName, const string& a_sXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset);

void FEncryptAesCtrData(void* a_pData, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataSize, n64 a_nXorOffset);

bool FEncryptXorData(void* a_pData, const char* a_pXorFileName, n64 a_nDataSize, n64 a_nXorOffset);

#endif	// _3DSCRYPT_H_
