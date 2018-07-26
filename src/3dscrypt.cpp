#include "3dscrypt.h"
#include <openssl/aes.h>
#include <openssl/modes.h>

void FEncryptAesCtrCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nSrcOffset, n64 a_nSize)
{
	u8 uKey[16] = {};
	a_Key.GetBytes(uKey, 16);
	u8 uCounter[16] = {};
	a_Counter.GetBytes(uCounter, 16);
	AES_KEY key;
	AES_set_encrypt_key(uKey, 128, &key);
	u8 uEcountBuf[16] = {};
	u32 uNum = 0;
	const n64 nBufferSize = 0x100000;
	u8* pInBuffer = new u8[nBufferSize];
	u8* pOutBuffer = new u8[nBufferSize];
	Fseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pInBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		CRYPTO_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
		fwrite(pOutBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pOutBuffer;
	delete[] pInBuffer;
}

bool FEncryptAesCtrFile(const UString& a_sDataFileName, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset)
{
	FILE* fpData = UFopen(a_sDataFileName.c_str(), USTR("rb+"));
	if (fpData == nullptr)
	{
		return false;
	}
	Fseek(fpData, 0, SEEK_END);
	n64 nDataSize = Ftell(fpData);
	if (nDataSize < a_nDataOffset)
	{
		fclose(fpData);
		UPrintf(USTR("ERROR: data file %") PRIUS USTR(" size less than data offset\n\n"), a_sDataFileName.c_str());
		return false;
	}
	if (a_bDataFileAll)
	{
		a_nDataSize = nDataSize - a_nDataOffset;
	}
	if (nDataSize < a_nDataOffset + a_nDataSize)
	{
		fclose(fpData);
		UPrintf(USTR("ERROR: data file %") PRIUS USTR(" size less than data offset + data size\n\n"), a_sDataFileName.c_str());
		return false;
	}
	u8 uKey[16] = {};
	a_Key.GetBytes(uKey, 16);
	CBigNum counter = a_Counter;
	counter += static_cast<int>(a_nXorOffset / 0x10);
	a_nXorOffset %= 0x10;
	n64 nXorOffset = a_nXorOffset;
	u8 uCounter[16] = {};
	counter.GetBytes(uCounter, 16);
	AES_KEY key;
	AES_set_encrypt_key(uKey, 128, &key);
	u8 uEcountBuf[16] = {};
	u32 uNum = 0;
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pInBuffer = new u8[nBufferSize];
	u8* pOutBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = nXorOffset + a_nDataSize > nBufferSize ? nBufferSize : nXorOffset + a_nDataSize;
		Fseek(fpData, a_nDataOffset + nIndex * nBufferSize - (a_nXorOffset - nXorOffset), SEEK_SET);
		fread(pInBuffer + nXorOffset, 1, static_cast<size_t>(nSize - nXorOffset), fpData);
		CRYPTO_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
		Fseek(fpData, a_nDataOffset + nIndex * nBufferSize - (a_nXorOffset - nXorOffset), SEEK_SET);
		fwrite(pOutBuffer + nXorOffset, 1, static_cast<size_t>(nSize - nXorOffset), fpData);
		a_nDataSize -= nSize - nXorOffset;
		nXorOffset = 0;
		nIndex++;
	}
	delete[] pOutBuffer;
	delete[] pInBuffer;
	fclose(fpData);
	return true;
}

bool FEncryptXorFile(const UString& a_sDataFileName, const UString& a_sXorFileName)
{
	FILE* fpData = UFopen(a_sDataFileName.c_str(), USTR("rb+"));
	if (fpData == nullptr)
	{
		return false;
	}
	Fseek(fpData, 0, SEEK_END);
	n64 nDataSize = Ftell(fpData);
	FILE* fpXor = UFopen(a_sXorFileName.c_str(), USTR("rb"));
	if (fpXor == nullptr)
	{
		fclose(fpData);
		return false;
	}
	Fseek(fpXor, 0, SEEK_END);
	n64 nXorSize = Ftell(fpXor);
	if (nXorSize < nDataSize)
	{
		fclose(fpXor);
		fclose(fpData);
		UPrintf(USTR("ERROR: xor file %") PRIUS USTR(" size less than data size\n\n"), a_sXorFileName.c_str());
		return false;
	}
	Fseek(fpXor, 0, SEEK_SET);
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	while (nDataSize > 0)
	{
		n64 nSize = nDataSize > nBufferSize ? nBufferSize : nDataSize;
		Fseek(fpData, nIndex * nBufferSize, SEEK_SET);
		fread(pDataBuffer, 1, static_cast<size_t>(nSize), fpData);
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(pDataBuffer);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = Align(nSize, 8) / 8;
		for (n64 i = 0; i < nXorCount; i++)
		{
			*pDataBuffer64++ ^= *pXorBuffer64++;
		}
		Fseek(fpData, nIndex * nBufferSize, SEEK_SET);
		fwrite(pDataBuffer, 1, static_cast<size_t>(nSize), fpData);
		nDataSize -= nSize;
		nIndex++;
	}
	delete[] pXorBuffer;
	delete[] pDataBuffer;
	fclose(fpXor);
	fclose(fpData);
	return true;
}

void FEncryptAesCtrData(void* a_pData, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataSize, n64 a_nXorOffset)
{
	u8 uKey[16] = {};
	a_Key.GetBytes(uKey, 16);
	CBigNum counter = a_Counter;
	counter += static_cast<int>(a_nXorOffset / 0x10);
	a_nXorOffset %= 0x10;
	u8 uCounter[16] = {};
	counter.GetBytes(uCounter, 16);
	AES_KEY key;
	AES_set_encrypt_key(uKey, 128, &key);
	u8 uEcountBuf[16] = {};
	u32 uNum = 0;
	if (a_nDataSize > 0)
	{
		if (a_nXorOffset == 0)
		{
			CRYPTO_ctr128_encrypt(reinterpret_cast<u8*>(a_pData), reinterpret_cast<u8*>(a_pData), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
		}
		else
		{
			const n64 nBufferSize = 0x10;
			u8 uBuffer[nBufferSize] = {};
			n64 nSize = a_nXorOffset + a_nDataSize > nBufferSize ? nBufferSize : a_nXorOffset + a_nDataSize;
			memcpy(uBuffer + a_nXorOffset, a_pData, static_cast<size_t>(nSize - a_nXorOffset));
			CRYPTO_ctr128_encrypt(uBuffer, uBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
			memcpy(a_pData, uBuffer + a_nXorOffset, static_cast<size_t>(nSize - a_nXorOffset));
			a_nDataSize -= nSize - a_nXorOffset;
			if (a_nDataSize > 0)
			{
				CRYPTO_ctr128_encrypt(reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum, reinterpret_cast<block128_f>(AES_encrypt));
			}
		}
	}
}
