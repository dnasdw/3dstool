#include "utility.h"

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

n32 FSToN32(const string& a_sString)
{
	return static_cast<n32>(strtol(a_sString.c_str(), nullptr, 10));
}

#if _3DSTOOL_COMPILER == COMPILER_MSC
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
#else
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
#endif

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	FFseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pBuffer, 1, (size_t)nSize, a_fpSrc);
		fwrite(pBuffer, 1, (size_t)nSize, a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pBuffer;
}

bool FCryptoFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset, bool a_bVervose)
{
	FILE* fpData = fopen(a_pDataFileName, "rb+");
	if (fpData == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pDataFileName);
		return false;
	}
	FILE* fpXor = fopen(a_pXorFileName, "rb");
	if (fpXor == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pXorFileName);
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
		if (a_bVervose)
		{
			printf("INFO: data file %s size less than data offset + data size\n\n", a_pDataFileName);
		}
		a_nDataSize = nDataSize - a_nDataOffset;
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
		fread(pDataBuffer, 1, (size_t)nSize, fpData);
		fread(pXorBuffer, 1, (size_t)nSize, fpXor);
		for (n64 i = 0; i < nSize; i++)
		{
			pDataBuffer[i] ^= pXorBuffer[i];
		}
		FFseek(fpData, a_nDataOffset + nIndex * nBufferSize, SEEK_SET);
		fwrite(pDataBuffer, 1, (size_t)nSize, fpData);
		a_nDataSize -= nSize;
		nIndex++;
	}
	delete[] pXorBuffer;
	delete[] pDataBuffer;
	fclose(fpXor);
	fclose(fpData);
	return true;
}

#if _3DSTOOL_COMPILER == COMPILER_MSC
bool MakeDir(const wchar_t* a_pDirName)
{
	if (_wmkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}
#else
bool MakeDir(const char* a_pDirName)
{
	if (mkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}
#endif

n64 FAlign(n64 a_nOffset, n64 a_nAlignment)
{
	return (a_nOffset + a_nAlignment - 1) / a_nAlignment * a_nAlignment;
}
