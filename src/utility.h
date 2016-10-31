#ifndef UTILITY_H_
#define UTILITY_H_

#define COMPILER_MSC  1
#define COMPILER_GNUC 2

#if defined(_MSC_VER)
#define _3DSTOOL_COMPILER COMPILER_MSC
#define _3DSTOOL_COMPILER_VERSION _MSC_VER
#else
#define _3DSTOOL_COMPILER COMPILER_GNUC
#define _3DSTOOL_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if _3DSTOOL_COMPILER == COMPILER_MSC
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>
#include <io.h>
#if _3DSTOOL_COMPILER_VERSION >= 1600
#include <codecvt>
#endif
#else
#if defined(_3DSTOOL_APPLE)
#include <mach-o/dyld.h>
#endif
#include <dirent.h>
#include <iconv.h>
#include <unistd.h>
#endif
#include <errno.h>
#if _3DSTOOL_COMPILER != COMPILER_MSC || (_3DSTOOL_COMPILER == COMPILER_MSC && _3DSTOOL_COMPILER_VERSION >= 1800)
#include <inttypes.h>
#else
#ifndef _PFX_8
#define _PFX_8 "hh"
#endif
#ifndef _PFX_16
#define _PFX_16 "h"
#endif
#ifndef _PFX_32
#define _PFX_32 "l"
#endif
#ifndef _PFX_64
#define _PFX_64 "ll"
#endif
#ifndef PRIu8
#define PRIu8 _PFX_8 "u"
#endif
#ifndef PRIu16
#define PRIu16 _PFX_16 "u"
#endif
#ifndef PRIu32
#define PRIu32 _PFX_32 "u"
#endif
#ifndef PRIu64
#define PRIu64 _PFX_64 "u"
#endif
#ifndef PRIX8
#define PRIX8 _PFX_8 "X"
#endif
#ifndef PRIX16
#define PRIX16 _PFX_16 "X"
#endif
#ifndef PRIX32
#define PRIX32 _PFX_32 "X"
#endif
#ifndef PRIX64
#define PRIX64 _PFX_64 "X"
#endif
#endif
#include <locale.h>
#include <stdarg.h>
#if _3DSTOOL_COMPILER != COMPILER_MSC || (_3DSTOOL_COMPILER == COMPILER_MSC && _3DSTOOL_COMPILER_VERSION >= 1600)
#include <stdint.h>
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <bitset>
#include <list>
#include <map>
#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include "bignum.h"

using namespace std;
#if _3DSTOOL_COMPILER == COMPILER_MSC && _3DSTOOL_COMPILER_VERSION < 1600
using namespace std::tr1;
#endif

typedef int8_t n8;
typedef int16_t n16;
typedef int32_t n32;
typedef int64_t n64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#if _3DSTOOL_COMPILER == COMPILER_MSC
#if _3DSTOOL_COMPILER_VERSION < 1600
#define nullptr NULL
#endif
#if _3DSTOOL_COMPILER_VERSION < 1600
typedef wchar_t Char16_t;
typedef std::wstring U16String;
#elif _3DSTOOL_COMPILER_VERSION >= 1900
typedef u16 Char16_t;
typedef std::basic_string<Char16_t> U16String;
#else
typedef char16_t Char16_t;
typedef u16string U16String;
#endif
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
typedef char16_t Char16_t;
typedef u16string U16String;
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

#define CONVERT_ENDIAN(n) (((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | (((n) & 0xFF00) << 8) | (((n) & 0xFF) << 24))

#define DNA_ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

void FSetLocale();

n32 FSToN32(const string& a_sString);

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
	static const n32 c_nBufferSize = 1024;
	static const n32 c_nConvertBufferSize = c_nBufferSize - 4;
	char buffer[c_nBufferSize];
	do
	{
		typename TSrc::value_type* pString = const_cast<typename TSrc::value_type*>(a_sString.c_str());
		char* pBuffer = buffer;
		size_t nBufferLeft = c_nConvertBufferSize;
		n32 nError = iconv(cvt, reinterpret_cast<char**>(&pString), &nStringLeft, &pBuffer, &nBufferLeft);
		if (nError == 0 || (nError == static_cast<size_t>(-1) && errno == E2BIG))
		{
			*reinterpret_cast<typename TDest::value_type*>(buffer + c_nConvertBufferSize - nBufferLeft) = 0;
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
string FSU16ToU8(const U16String& a_sString);
wstring FSU8ToW(const string& a_sString);
wstring FSAToW(const string& a_sString);
wstring FSU16ToW(const U16String& a_sString);
U16String FSU8ToU16(const string& a_sString);
U16String FSWToU16(const wstring& a_sString);

#if _3DSTOOL_COMPILER == COMPILER_MSC
#define FSAToUnicode(x) FSAToW(x)
#define FSU16ToUnicode(x) FSU16ToW(x)
#define FSUnicodeToU16(x) FSWToU16(x)
#else
#define FSAToUnicode(x) string(x)
#define FSU16ToUnicode(x) FSU16ToU8(x)
#define FSUnicodeToU16(x) FSU8ToU16(x)
#endif

string FFormatV(const char* a_szFormat, va_list a_vaList);
wstring FFormatV(const wchar_t* a_szFormat, va_list a_vaList);
string FFormat(const char* a_szFormat, ...);
wstring FFormat(const wchar_t* a_szFormat, ...);

template<typename T>
bool FSStartsWith(const T& a_sString, const T& a_sPrefix, typename T::size_type a_uStart = 0)
{
	if (a_uStart > a_sString.size())
	{
		return false;
	}
	return a_sString.compare(a_uStart, a_sPrefix.size(), a_sPrefix) == 0;
}

template<typename T>
bool FSEndsWith(const T& a_sString, const T& a_sSuffix)
{
	if (a_sString.size() < a_sSuffix.size())
	{
		return false;
	}
	return a_sString.compare(a_sString.size() - a_sSuffix.size(), a_sSuffix.size(), a_sSuffix) == 0;
}

template<typename T>
T FSTrim(const T& a_sString)
{
	typename T::size_type uSize = a_sString.size();
	typename T::size_type uPos = 0;
	while (uPos < uSize && a_sString[uPos] >= 0 && a_sString[uPos] <= static_cast<typename T::value_type>(' '))
	{
		uPos++;
	}
	while (uPos < uSize && a_sString[uSize - 1] >= 0 && a_sString[uSize - 1] <= static_cast<typename T::value_type>(' '))
	{
		uSize--;
	}
	return (uPos > 0 || uSize < a_sString.size()) ? a_sString.substr(uPos, uSize - uPos) : a_sString;
}

template<typename T>
vector<T> FSSplit(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	for (typename T::size_type uOffset = 0; uOffset < a_sString.size(); uOffset += a_sSeparator.size())
	{
		typename T::size_type uPos = a_sString.find(a_sSeparator, uOffset);
		if (uPos != T::npos)
		{
			vString.push_back(a_sString.substr(uOffset, uPos - uOffset));
			uOffset = uPos;
		}
		else
		{
			vString.push_back(a_sString.substr(uOffset));
			break;
		}
	}
	return vString;
}

template<typename T>
vector<T> FSSplitOf(const T& a_sString, const T& a_sSeparatorSet)
{
	vector<T> vString;
	for (typename T::const_iterator it = a_sString.begin(); it != a_sString.end(); ++it)
	{
		typename T::const_iterator itPos = find_first_of(it, a_sString.end(), a_sSeparatorSet.begin(), a_sSeparatorSet.end());
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

const String& FGetModuleFile();

const String& FGetModuleDir();

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize);

void FEncryptAesCtrCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nSrcOffset, n64 a_nSize);

bool FEncryptXorCopyFile(FILE* a_fpDest, FILE* a_fpSrc, const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize);

void FPadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData);

bool FEncryptAesCtrFile(const char* a_pDataFileName, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset);

bool FEncryptXorFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset);

void FEncryptAesCtrData(void* a_pData, const CBigNum& a_Key, const CBigNum& a_Counter, n64 a_nDataSize, n64 a_nXorOffset);

bool FEncryptXorData(void* a_pData, const char* a_pXorFileName, n64 a_nDataSize, n64 a_nXorOffset);

bool FGetFileSize(const String::value_type* a_pFileName, n64& a_nFileSize);

bool FMakeDir(const String::value_type* a_pDirName);

FILE* FFopenA(const char* a_pFileName, const char* a_pMode);

#if _3DSTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode);
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset);

n64 FAlign(n64 a_nOffset, n64 a_nAlignment);

#endif	// UTILITY_H_
