#include "utility.h"
#include <openssl/aes.h>

#if _3DSTOOL_COMPILER != COMPILER_MSC
string g_sLocaleName = "";
#endif

void FSetLocale()
{
	string sLocale = setlocale(LC_ALL, "");
#if _3DSTOOL_COMPILER != COMPILER_MSC
	vector<string> vLocale = FSSplit<string>(sLocale, ".");
	if (!vLocale.empty())
	{
		g_sLocaleName = vLocale[vLocale.size() - 1];
	}
#endif
}

u8 FCHexToU8(char a_cHex)
{
	if (a_cHex >= '0' && a_cHex <= '9')
	{
		return a_cHex - '0';
	}
	else if (a_cHex >= 'A' && a_cHex <= 'F')
	{
		return a_cHex - 'A' + 10;
	}
	else if (a_cHex >= 'a' && a_cHex <= 'f')
	{
		return a_cHex - 'a' + 10;
	}
	return 0;
}

bool FSHexToU8(string a_sHex, u8* a_pArray)
{
	if (a_sHex.size() % 2 != 0)
	{
		return false;
	}
	for (int i = 0; i < static_cast<int>(a_sHex.size()); i += 2)
	{
		*a_pArray++ = (FCHexToU8(a_sHex[i]) << 4 & 0xF0) | (FCHexToU8(a_sHex[i + 1]) & 0xF);
	}
	return true;
}

n32 FSToN32(const string& a_sString)
{
	return static_cast<n32>(strtol(a_sString.c_str(), nullptr, 10));
}

#if _3DSTOOL_COMPILER == COMPILER_MSC
string FSWToU8(const wstring& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.to_bytes(a_sString);
}

string FSU16ToU8(const u16string& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.to_bytes(a_sString);
}

wstring FSU8ToW(const string& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.from_bytes(a_sString);
}

wstring FSAToW(const string& a_sString)
{
	static wstring_convert<codecvt<wchar_t, char, mbstate_t>> c_cvt_a(new codecvt<wchar_t, char, mbstate_t>(""));
	return c_cvt_a.from_bytes(a_sString);
}

wstring FSU16ToW(const u16string& a_sString)
{
	return FSU8ToW(FSU16ToU8(a_sString));
}

u16string FSU8ToU16(const string& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.from_bytes(a_sString);
}

u16string FSWToU16(const wstring& a_sString)
{
	return FSU8ToU16(FSWToU8(a_sString));
}
#else
string FSWToU8(const wstring& a_sString)
{
	return FSTToT<wstring, string>(a_sString, "WCHAR_T", "UTF-8");
}

string FSU16ToU8(const u16string& a_sString)
{
	return FSTToT<u16string, string>(a_sString, "UTF-16LE", "UTF-8");
}

wstring FSU8ToW(const string& a_sString)
{
	return FSTToT<string, wstring>(a_sString, "UTF-8", "WCHAR_T");
}

wstring FSAToW(const string& a_sString)
{
	return FSTToT<string, wstring>(a_sString, g_sLocaleName, "WCHAR_T");
}

wstring FSU16ToW(const u16string& a_sString)
{
	return FSTToT<u16string, wstring>(a_sString, "UTF-16LE", "WCHAR_T");
}

u16string FSU8ToU16(const string& a_sString)
{
	return FSTToT<string, u16string>(a_sString, "UTF-8", "UTF-16LE");
}

u16string FSWToU16(const wstring& a_sString)
{
	return FSTToT<wstring, u16string>(a_sString, "WCHAR_T", "UTF-16LE");
}
#endif

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pBuffer;
}

void FCryptoAesCtrCopyFile(FILE* a_fpDest, FILE* a_fpSrc, u8 a_uKey[16], u8 a_uAesCtr[16], n64 a_nSrcOffset, n64 a_nSize, bool a_bVerbose)
{
	AES_KEY aesKey;
	AES_set_encrypt_key(a_uKey, 128, &aesKey);
	u8 uEcountBuf[16] = {};
	u32 nNum = 0;
	const n64 nBufferSize = 0x100000;
	u8* pInBuffer = new u8[nBufferSize];
	u8* pOutBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pInBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		AES_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &aesKey, a_uAesCtr, uEcountBuf, &nNum);
		fwrite(pOutBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pOutBuffer;
	delete[] pInBuffer;
}

bool FCryptoXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const char* a_pXorFileName, n64 a_nSrcOffset, n64 a_nSize, n64 a_nXorOffset, bool a_bVerbose)
{
	FILE* fpXor = FFopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	FFseek(fpXor, 0, SEEK_END);
	n64 nXorSize = FFtell(fpXor);
	if (nXorSize - a_nXorOffset < a_nSize && a_bVerbose)
	{
		printf("INFO: xor file %s size less than data size\n\n", a_pXorFileName);
	}
	FFseek(fpXor, a_nXorOffset, SEEK_SET);
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pDataBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(pDataBuffer);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = FAlign(nSize, 8) / 8;
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

void FPadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	memset(pBuffer, a_uPadData, nBufferSize);
	while (a_nPadSize > 0)
	{
		n64 nSize = a_nPadSize > nBufferSize ? nBufferSize : a_nPadSize;
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpFile);
		a_nPadSize -= nSize;
	}
	delete[] pBuffer;
}

bool FCryptoAesCtrFile(const char* a_pDataFileName, u8 a_uKey[16], u8 a_uAesCtr[16], n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, bool a_bVerbose)
{
	FILE* fpData = FFopen(a_pDataFileName, "rb+");
	if (fpData == nullptr)
	{
		return false;
	}
	FFseek(fpData, 0, SEEK_END);
	n64 nDataSize = FFtell(fpData);
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
		if (a_bVerbose)
		{
			printf("INFO: data file %s size less than data offset + data size\n\n", a_pDataFileName);
		}
		a_nDataSize = nDataSize - a_nDataOffset;
	}
	AES_KEY aesKey;
	AES_set_encrypt_key(a_uKey, 128, &aesKey);
	u8 uEcountBuf[16] = {};
	u32 nNum = 0;
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pInBuffer = new u8[nBufferSize];
	u8* pOutBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = a_nDataSize > nBufferSize ? nBufferSize : a_nDataSize;
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fread(pInBuffer, 1, static_cast<size_t>(nSize), fpData);
		AES_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &aesKey, a_uAesCtr, uEcountBuf, &nNum);
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fwrite(pOutBuffer, 1, static_cast<size_t>(nSize), fpData);
		a_nDataSize -= nSize;
		nIndex++;
	}
	delete[] pOutBuffer;
	delete[] pInBuffer;
	fclose(fpData);
	return true;
}

bool FCryptoXorFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset, bool a_bVerbose)
{
	FILE* fpData = FFopen(a_pDataFileName, "rb+");
	if (fpData == nullptr)
	{
		return false;
	}
	FILE* fpXor = FFopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	FFseek(fpData, 0, SEEK_END);
	n64 nDataSize = FFtell(fpData);
	if (nDataSize < a_nDataOffset)
	{
		fclose(fpXor);
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
		if (a_bVerbose)
		{
			printf("INFO: data file %s size less than data offset + data size\n\n", a_pDataFileName);
		}
		a_nDataSize = nDataSize - a_nDataOffset;
	}
	FFseek(fpXor, 0, SEEK_END);
	n64 nXorSize = FFtell(fpXor);
	if (nXorSize - a_nXorOffset < a_nDataSize)
	{
		if (a_bVerbose)
		{
			printf("INFO: xor file %s size less than data size\n\n", a_pXorFileName);
		}
		a_nDataSize = nXorSize - a_nXorOffset;
	}
	FFseek(fpXor, a_nXorOffset, SEEK_SET);
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = a_nDataSize > nBufferSize ? nBufferSize : a_nDataSize;
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fread(pDataBuffer, 1, static_cast<size_t>(nSize), fpData);
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(pDataBuffer);
		u64* pXorBuffer64 = reinterpret_cast<u64*>(pXorBuffer);
		n64 nXorCount = FAlign(nSize, 8) / 8;
		for (n64 i = 0; i < nXorCount; i++)
		{
			*pDataBuffer64++ ^= *pXorBuffer64++;
		}
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
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

bool FGetFileSize(const String::value_type* a_pFileName, n64& a_nFileSize)
{
	SStat st;
	if (FStat(a_pFileName, &st) != 0)
	{
		a_nFileSize = 0;
		return false;
	}
	a_nFileSize = st.st_size;
	return true;
}

bool FMakeDir(const String::value_type* a_pDirName)
{
	if (FMkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}

FILE* FFopenA(const char* a_pFileName, const char* a_pMode)
{
	FILE* fp = fopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}

#if _3DSTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode)
{
	FILE* fp = _wfopen(a_pFileName, a_pMode);
	if (fp == nullptr)
	{
		wprintf(L"ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset)
{
	if (fflush(a_fpFile) != 0)
	{
		return false;
	}
	int nFd = FFileno(a_fpFile);
	if (nFd == -1)
	{
		return false;
	}
	FFseek(a_fpFile, 0, SEEK_END);
	n64 nFileSize = FFtell(a_fpFile);
	if (nFileSize < a_nOffset)
	{
		n64 nOffset = FLseek(nFd, a_nOffset - 1, SEEK_SET);
		if (nOffset == -1)
		{
			return false;
		}
		fputc(0, a_fpFile);
		fflush(a_fpFile);
	}
	else
	{
		FFseek(a_fpFile, a_nOffset, SEEK_SET);
	}
	return true;
}

n64 FAlign(n64 a_nOffset, n64 a_nAlignment)
{
	return (a_nOffset + a_nAlignment - 1) / a_nAlignment * a_nAlignment;
}

size_t FU16Strlen(const char16_t* a_pString)
{
	size_t nLength = 0;
	while (*a_pString++ != 0)
	{
		nLength++;
	}
	return nLength;
}
