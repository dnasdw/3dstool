#ifndef UTILITY_H_
#define UTILITY_H_

#define COMPILER_MSC  1
#define COMPILER_GNUC 2

#if defined(_MSC_VER)
#define _3DSTOOL_COMPILER COMPILER_MSC
#else
#define _3DSTOOL_COMPILER COMPILER_GNUC
#endif

#if _3DSTOOL_COMPILER == COMPILER_MSC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <codecvt>
#else
#if defined(_3DSTOOL_APPLE)
#include <mach-o/dyld.h>
#endif
#include <dirent.h>
#include <iconv.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <list>
#include <map>
#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

typedef int8_t n8;
typedef int16_t n16;
typedef int32_t n32;
typedef int64_t n64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if _3DSTOOL_COMPILER == COMPILER_MSC
typedef wstring String;
typedef wregex Regex;
typedef struct _stat64 SStat;
#define STR(x) L##x
#define FStat _wstat64
#define FMkdir _wmkdir
#define FFopen FFopenA
#define FFopenUnicode FFopenW
#define FFseek _fseeki64
#define FFtell _ftelli64
#define FFileno _fileno
#define FLseek _lseeki64
#define FChsize _chsize_s
#define FPrintf wprintf
#define MSC_PUSH_PACKED <pshpack1.h>
#define MSC_POP_PACKED <poppack.h>
#define GNUC_PACKED
#else
typedef string String;
typedef regex Regex;
typedef struct stat SStat;
#define STR(x) x
#define FStat stat
#define FMkdir(x) mkdir((x), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define FFopen FFopenA
#define FFopenUnicode FFopenA
#define FFseek fseeko
#define FFtell ftello
#define FFileno fileno
#define FLseek lseek
#define FChsize ftruncate
#define FPrintf printf
#define MSC_PUSH_PACKED <stdlib.h>
#define MSC_POP_PACKED <stdlib.h>
#define GNUC_PACKED __attribute__((packed))
#endif

#define CONVERT_ENDIAN(n) ((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | (((n) & 0xFF00) << 8) | (((n) & 0xFF) << 24)

void FSetLocale();

u8 FCHexToU8(char a_cHex);

bool FSHexToU8(string a_sHex, u8* a_pArray);

n32 FSToN32(const string& a_sString);

template<typename T>
T FSTrim(const T& a_sString)
{
	typename T::size_type stSize = a_sString.size();
	typename T::size_type st = 0;
	while (st < stSize && a_sString[st] >= 0 && a_sString[st] <= static_cast<typename T::value_type>(' '))
	{
		st++;
	}
	while (st < stSize && a_sString[stSize - 1] >= 0 && a_sString[stSize - 1] <= static_cast<typename T::value_type>(' '))
	{
		stSize--;
	}
	return (st > 0 || stSize < a_sString.size()) ? a_sString.substr(st, stSize - st) : a_sString;
}

template<typename T>
vector<T> FSSplit(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	for (typename T::size_type nOffset = 0; nOffset < a_sString.size(); nOffset += a_sSeparator.size())
	{
		typename T::size_type nPos = a_sString.find(a_sSeparator, nOffset);
		if (nPos != T::npos)
		{
			vString.push_back(a_sString.substr(nOffset, nPos - nOffset));
			nOffset = nPos;
		}
		else
		{
			vString.push_back(a_sString.substr(nOffset));
			break;
		}
	}
	return vString;
}

template<typename T>
vector<T> FSSplitOf(const T& a_sString, const T& a_sSeparatorSet)
{
	vector<T> vString;
	for (auto it = a_sString.begin(); it != a_sString.end(); ++it)
	{
		auto itPos = find_first_of(it, a_sString.end(), a_sSeparatorSet.begin(), a_sSeparatorSet.end());
		if (itPos != a_sString.end())
		{
			vString.push_back(a_sString.substr(it - a_sString.begin(), itPos - it));
			it = itPos;
		}
		else
		{
			vString.push_back(a_sString.substr(it - a_sString.begin()));
			break;
		}
	}
	return vString;
}

#if _3DSTOOL_COMPILER != COMPILER_MSC
template<typename TSrc, typename TDest>
TDest FSTToT(const TSrc& a_sString, const string& a_sSrcType, const string& a_sDestType)
{
	TDest sConverted;
	iconv_t cvt = iconv_open(a_sDestType.c_str(), a_sSrcType.c_str());
	if (cvt == reinterpret_cast<iconv_t>(-1))
	{
		return sConverted;
	}
	size_t nStringLeft = a_sString.size() * sizeof(typename TSrc::value_type);
	static const n32 c_kBufferSize = 1024;
	static const n32 c_kConvertBufferSize = c_kBufferSize - 4;
	char buffer[c_kBufferSize];
	do
	{
		typename TSrc::value_type* pString = const_cast<typename TSrc::value_type*>(a_sString.c_str());
		char* pBuffer = buffer;
		size_t nBufferLeft = c_kConvertBufferSize;
		n32 nError = iconv(cvt, reinterpret_cast<char**>(&pString), &nStringLeft, &pBuffer, &nBufferLeft);
		if (nError == 0 || (nError == static_cast<size_t>(-1) && errno == E2BIG))
		{
			*reinterpret_cast<typename TDest::value_type*>(buffer + c_kConvertBufferSize - nBufferLeft) = 0;
			sConverted += reinterpret_cast<typename TDest::value_type*>(buffer);
			if (nError == 0)
			{
				break;
			}
		}
		else
		{
			break;
		}
	} while (true);
	iconv_close(cvt);
	return sConverted;
}
#endif

string FSWToU8(const wstring& a_sString);
string FSU16ToU8(const u16string& a_sString);
wstring FSU8ToW(const string& a_sString);
wstring FSAToW(const string& a_sString);
wstring FSU16ToW(const u16string& a_sString);
u16string FSU8ToU16(const string& a_sString);
u16string FSWToU16(const wstring& a_sString);

#if _3DSTOOL_COMPILER == COMPILER_MSC
#define FSAToUnicode(x) FSAToW(x)
#define FSU16ToUnicode(x) FSU16ToW(x)
#define FSUnicodeToU16(x) FSWToU16(x)
#else
#define FSAToUnicode(x) (x)
#define FSU16ToUnicode(x) FSU16ToU8(x)
#define FSUnicodeToU16(x) FSU8ToU16(x)
#endif

template<typename T>
bool FSStartsWith(const T& a_sString, const T& a_sPrefix, typename T::size_type a_stStart = 0)
{
	if (a_stStart > a_sString.size())
	{
		return false;
	}
	return a_sString.compare(a_stStart, a_sPrefix.size(), a_sPrefix) == 0;
}

const String& FGetModuleDir();

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize);

void FEncryptAesCtrCopyFile(FILE* a_fpDest, FILE* a_fpSrc, u8 a_uKey[16], u8 a_uAesCtr[16], n64 a_nSrcOffset, n64 a_nSize, bool a_bVerbose);

bool FEncryptXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, bool a_bVerbose);

void FPadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData);

bool FEncryptAesCtrFile(const char* a_pDataFileName, u8 a_uKey[16], u8 a_uAesCtr[16], n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, bool a_bVerbose);

bool FEncryptXorFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset, bool a_bVerbose);

bool FEncryptXorData(void* a_pData, const char* a_pXorFileName, n64 a_nDataSize, n64 a_nXorOffset, bool a_bVerbose);

bool FGetFileSize(const String::value_type* a_pFileName, n64& a_nFileSize);

bool FMakeDir(const String::value_type* a_pDirName);

FILE* FFopenA(const char* a_pFileName, const char* a_pMode);

#if _3DSTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode);
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset);

n64 FAlign(n64 a_nOffset, n64 a_nAlignment);

#endif	// UTILITY_H_
