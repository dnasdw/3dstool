#include "3dscrypt.h"
#include <openssl/aes.h>

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
		AES_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum);
		fwrite(pOutBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pOutBuffer;
	delete[] pInBuffer;
}

bool FEncryptXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const string& a_sXorFileName, n64 a_nOffset, n64 a_nSize)
{
	FILE* fpXor = Fopen(a_sXorFileName.c_str(), "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	Fseek(fpXor, 0, SEEK_END);
	n64 nXorSize = Ftell(fpXor);
	if (nXorSize < a_nSize)
	{
		fclose(fpXor);
		printf("ERROR: xor file %s size less than data size\n\n", a_sXorFileName.c_str());
		return false;
	}
	Fseek(fpXor, 0, SEEK_SET);
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	Fseek(a_fpSrc, a_nOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pDataBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(pDataBuffer);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = Align(nSize, 8) / 8;
		for (n64 i = 0; i < nXorCount; i++)
		{
			*pDataBuffer64++ ^= *pXorBuffer64++;
		}
		fwrite(pDataBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pXorBuffer;
	delete[] pDataBuffer;
	fclose(fpXor);
	return true;
}

bool FEncryptAesCtrFile(const char* a_pDataFileName, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset)
{
	FILE* fpData = Fopen(a_pDataFileName, "rb+");
	if (fpData == nullptr)
	{
		return false;
	}
	Fseek(fpData, 0, SEEK_END);
	n64 nDataSize = Ftell(fpData);
	if (nDataSize < a_nDataOffset)
	{
		fclose(fpData);
		printf("ERROR: data file %s size less than data offset\n\n", a_pDataFileName);
		return false;
	}
	if (a_bDataFileAll)
	{
		a_nDataSize = nDataSize - a_nDataOffset;
	}
	if (nDataSize < a_nDataOffset + a_nDataSize)
	{
		fclose(fpData);
		printf("ERROR: data file %s size less than data offset + data size\n\n", a_pDataFileName);
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
		AES_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum);
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

bool FEncryptXorFile(const string& a_sDataFileName, const string& a_sXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset)
{
	FILE* fpData = Fopen(a_sDataFileName.c_str(), "rb+");
	if (fpData == nullptr)
	{
		return false;
	}
	Fseek(fpData, 0, SEEK_END);
	n64 nDataSize = Ftell(fpData);
	if (nDataSize < a_nDataOffset)
	{
		fclose(fpData);
		printf("ERROR: data file %s size less than data offset\n\n", a_sDataFileName.c_str());
		return false;
	}
	if (a_bDataFileAll)
	{
		a_nDataSize = nDataSize - a_nDataOffset;
	}
	if (nDataSize < a_nDataOffset + a_nDataSize)
	{
		fclose(fpData);
		printf("ERROR: data file %s size less than data offset + data size\n\n", a_sDataFileName.c_str());
		return false;
	}
	FILE* fpXor = Fopen(a_sXorFileName.c_str(), "rb");
	if (fpXor == nullptr)
	{
		fclose(fpData);
		return false;
	}
	Fseek(fpXor, 0, SEEK_END);
	n64 nXorSize = Ftell(fpXor);
	if (nXorSize - a_nXorOffset < a_nDataSize)
	{
		fclose(fpXor);
		fclose(fpData);
		printf("ERROR: xor file %s size less than data size\n\n", a_sXorFileName.c_str());
		return false;
	}
	Fseek(fpXor, a_nXorOffset, SEEK_SET);
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = a_nDataSize > nBufferSize ? nBufferSize : a_nDataSize;
		Fseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fread(pDataBuffer, 1, static_cast<size_t>(nSize), fpData);
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(pDataBuffer);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = Align(nSize, 8) / 8;
		for (n64 i = 0; i < nXorCount; i++)
		{
			*pDataBuffer64++ ^= *pXorBuffer64++;
		}
		Fseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fwrite(pDataBuffer, 1, static_cast<size_t>(nSize), fpData);
		a_nDataSize -= nSize;
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
			AES_ctr128_encrypt(reinterpret_cast<u8*>(a_pData), reinterpret_cast<u8*>(a_pData), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum);
		}
		else
		{
			const n64 nBufferSize = 0x10;
			u8 uBuffer[nBufferSize] = {};
			n64 nSize = a_nXorOffset + a_nDataSize > nBufferSize ? nBufferSize : a_nXorOffset + a_nDataSize;
			memcpy(uBuffer + a_nXorOffset, a_pData, static_cast<size_t>(nSize - a_nXorOffset));
			AES_ctr128_encrypt(uBuffer, uBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum);
			memcpy(a_pData, uBuffer + a_nXorOffset, static_cast<size_t>(nSize - a_nXorOffset));
			a_nDataSize -= nSize - a_nXorOffset;
			if (a_nDataSize > 0)
			{
				AES_ctr128_encrypt(reinterpret_cast<u8*>(a_pData)+(nSize - a_nXorOffset), reinterpret_cast<u8*>(a_pData)+(nSize - a_nXorOffset), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum);
			}
		}
	}
}

bool FEncryptXorData(void* a_pData, const string& a_sXorFileName, n64 a_nDataSize, n64 a_nXorOffset)
{
	FILE* fpXor = Fopen(a_sXorFileName.c_str(), "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	Fseek(fpXor, 0, SEEK_END);
	n64 nXorSize = Ftell(fpXor);
	if (nXorSize - a_nXorOffset < a_nDataSize)
	{
		fclose(fpXor);
		printf("ERROR: xor file %s size less than data size\n\n", a_sXorFileName.c_str());
		return false;
	}
	Fseek(fpXor, a_nXorOffset, SEEK_SET);
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pXorBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = a_nDataSize > nBufferSize ? nBufferSize : a_nDataSize;
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(static_cast<u8*>(a_pData)+nIndex * nBufferSize);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = nSize / 8;
		for (n64 i = 0; i < nXorCount; i++)
		{
			*pDataBuffer64++ ^= *pXorBuffer64++;
		}
		int nRemain = nSize % 8;
		if (nRemain != 0)
		{
			u8* pDataBuffer8 = reinterpret_cast<u8*>(pDataBuffer64);
			u8* pXorBuffer8 = reinterpret_cast<u8*>(pXorBuffer64);
			for (n64 i = 0; i < nRemain; i++)
			{
				*pDataBuffer8++ ^= *pXorBuffer8++;
			}
		}
		a_nDataSize -= nSize;
		nIndex++;
	}
	delete[] pXorBuffer;
	fclose(fpXor);
	return true;
}
