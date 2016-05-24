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
		g_sLocaleName = vLocale.back();
	}
#endif
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

string FSU16ToU8(const U16String& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
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

wstring FSU16ToW(const U16String& a_sString)
{
	return FSU8ToW(FSU16ToU8(a_sString));
}

U16String FSU8ToU16(const string& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.from_bytes(a_sString);
}

U16String FSWToU16(const wstring& a_sString)
{
	return FSU8ToU16(FSWToU8(a_sString));
}
#else
string FSWToU8(const wstring& a_sString)
{
	return FSTToT<wstring, string>(a_sString, "WCHAR_T", "UTF-8");
}

string FSU16ToU8(const U16String& a_sString)
{
	return FSTToT<U16String, string>(a_sString, "UTF-16LE", "UTF-8");
}

wstring FSU8ToW(const string& a_sString)
{
	return FSTToT<string, wstring>(a_sString, "UTF-8", "WCHAR_T");
}

wstring FSAToW(const string& a_sString)
{
	return FSTToT<string, wstring>(a_sString, g_sLocaleName, "WCHAR_T");
}

wstring FSU16ToW(const U16String& a_sString)
{
	return FSTToT<U16String, wstring>(a_sString, "UTF-16LE", "WCHAR_T");
}

U16String FSU8ToU16(const string& a_sString)
{
	return FSTToT<string, U16String>(a_sString, "UTF-8", "UTF-16LE");
}

U16String FSWToU16(const wstring& a_sString)
{
	return FSTToT<wstring, U16String>(a_sString, "WCHAR_T", "UTF-16LE");
}
#endif

static const n32 s_nFormatBufferSize = 4096;

string FFormatV(const char* a_szFormat, va_list a_vaList)
{
	static char c_szBuffer[s_nFormatBufferSize] = {};
	vsnprintf(c_szBuffer, s_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

wstring FFormatV(const wchar_t* a_szFormat, va_list a_vaList)
{
	static wchar_t c_szBuffer[s_nFormatBufferSize] = {};
	vswprintf(c_szBuffer, s_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

string FFormat(const char* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	string sFormatted = FFormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}

wstring FFormat(const wchar_t* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	wstring sFormatted = FFormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}

const String& FGetModuleFile()
{
	const int nMaxPath = 4096;
	static String sFile;
	sFile.clear();
	sFile.resize(nMaxPath, STR('\0'));
	size_t uSize = 0;
#if _3DSTOOL_COMPILER == COMPILER_MSC
	uSize = GetModuleFileNameW(nullptr, &*sFile.begin(), nMaxPath);
#elif defined(_3DSTOOL_APPLE)
	char path[nMaxPath] = {};
	u32 uPathSize = static_cast<u32>(sizeof(path));
	if (_NSGetExecutablePath(path, &uPathSize) != 0)
	{
		printf("ERROR: _NSGetExecutablePath error\n\n");
		sFile.erase();
	}
	else if (realpath(path, &sFile.front()) == nullptr)
	{
		sFile.erase();
	}
	uSize = strlen(sFile.c_str());
#else
	ssize_t nCount = readlink("/proc/self/exe", &sFile.front(), nMaxPath);
	if (nCount == -1)
	{
		printf("ERROR: readlink /proc/self/exe error\n\n");
		sFile.erase();
	}
	else
	{
		sFile[nCount] = '\0';
	}
	uSize = strlen(sFile.c_str());
#endif
	sFile.erase(uSize);
	replace(sFile.begin(), sFile.end(), STR('\\'), STR('/'));
	return sFile;
}

const String& FGetModuleDir()
{
	static String sDir = FGetModuleFile();
	String::size_type nPos = sDir.rfind(STR('/'));
	if (nPos != String::npos)
	{
		sDir.erase(nPos);
	}
	else
	{
		sDir.erase();
	}
	return sDir;
}

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
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
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

bool FEncryptXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize)
{
	FILE* fpXor = FFopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	FFseek(fpXor, 0, SEEK_END);
	n64 nXorSize = FFtell(fpXor);
	if (nXorSize < a_nSize)
	{
		fclose(fpXor);
		printf("ERROR: xor file %s size less than data size\n\n", a_pXorFileName);
		return false;
	}
	FFseek(fpXor, 0, SEEK_SET);
	const n64 nBufferSize = 0x100000;
	u8* pDataBuffer = new u8[nBufferSize];
	u8* pXorBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nOffset, SEEK_SET);
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

bool FEncryptAesCtrFile(const char* a_pDataFileName, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset)
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
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize - (a_nXorOffset - nXorOffset), SEEK_SET);
		fread(pInBuffer + nXorOffset, 1, static_cast<size_t>(nSize - nXorOffset), fpData);
		AES_ctr128_encrypt(pInBuffer, pOutBuffer, static_cast<size_t>(nSize), &key, uCounter, uEcountBuf, &uNum);
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize - (a_nXorOffset - nXorOffset), SEEK_SET);
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

bool FEncryptXorFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset)
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
		fclose(fpData);
		printf("ERROR: data file %s size less than data offset + data size\n\n", a_pDataFileName);
		return false;
	}
	FILE* fpXor = FFopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		fclose(fpData);
		return false;
	}
	FFseek(fpXor, 0, SEEK_END);
	n64 nXorSize = FFtell(fpXor);
	if (nXorSize - a_nXorOffset < a_nDataSize)
	{
		fclose(fpXor);
		fclose(fpData);
		printf("ERROR: xor file %s size less than data size\n\n", a_pXorFileName);
		return false;
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
				AES_ctr128_encrypt(reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), reinterpret_cast<u8*>(a_pData) + (nSize - a_nXorOffset), static_cast<size_t>(a_nDataSize), &key, uCounter, uEcountBuf, &uNum);
			}
		}
	}
}

bool FEncryptXorData(void* a_pData, const char* a_pXorFileName, n64 a_nDataSize, n64 a_nXorOffset)
{
	FILE* fpXor = FFopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		return false;
	}
	FFseek(fpXor, 0, SEEK_END);
	n64 nXorSize = FFtell(fpXor);
	if (nXorSize - a_nXorOffset < a_nDataSize)
	{
		fclose(fpXor);
		printf("ERROR: xor file %s size less than data size\n\n", a_pXorFileName);
		return false;
	}
	FFseek(fpXor, a_nXorOffset, SEEK_SET);
	n64 nIndex = 0;
	const n64 nBufferSize = 0x100000;
	u8* pXorBuffer = new u8[nBufferSize];
	while (a_nDataSize > 0)
	{
		n64 nSize = a_nDataSize > nBufferSize ? nBufferSize : a_nDataSize;
		fread(pXorBuffer, 1, static_cast<size_t>(nSize), fpXor);
		u64* pDataBuffer64 = reinterpret_cast<u64*>(static_cast<u8*>(a_pData) + nIndex * nBufferSize);
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
