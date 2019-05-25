#ifndef UTILITY_H_
#define UTILITY_H_

#define SDW_COMPILER_MSC   1
#define SDW_COMPILER_GNUC  2
#define SDW_COMPILER_CLANG 3

#if defined(_MSC_VER)
#define SDW_COMPILER SDW_COMPILER_MSC
#define SDW_COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
#define SDW_COMPILER SDW_COMPILER_CLANG
#define SDW_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#define SDW_COMPILER SDW_COMPILER_GNUC
#define SDW_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#error "compiler no support"
#endif

#define SDW_PLATFORM_WINDOWS 1
#define SDW_PLATFORM_LINUX   2
#define SDW_PLATFORM_MACOS   3
#define SDW_PLATFORM_CYGWIN  4

#if defined(_WIN32)
#define SDW_PLATFORM SDW_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define SDW_PLATFORM SDW_PLATFORM_MACOS
#elif defined(__linux__)
#define SDW_PLATFORM SDW_PLATFORM_LINUX
#elif defined(__CYGWIN__)
#define SDW_PLATFORM SDW_PLATFORM_CYGWIN
#else
#error "platform no support"
#endif

#if SDW_COMPILER == SDW_COMPILER_MSC
#define SDW_MSC_PUSH_PACKED <pshpack1.h>
#define SDW_MSC_POP_PACKED <poppack.h>
#define SDW_GNUC_PACKED
#else
#define SDW_MSC_PUSH_PACKED <cstdlib>
#define SDW_MSC_POP_PACKED <cstdlib>
#define SDW_GNUC_PACKED __attribute__((packed))
#endif

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>
#include <io.h>
#if defined(SDW_MAIN)
#include <shellapi.h>
#endif
#else
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
#include <mach-o/dyld.h>
#endif
#if (SDW_COMPILER == SDW_COMPILER_GNUC && SDW_COMPILER_VERSION < 50400) || SDW_PLATFORM == SDW_PLATFORM_CYGWIN || defined(SDW_XCONVERT)
#include <iconv.h>
#endif
#include <dirent.h>
#include <unistd.h>
#endif
#include <sys/stat.h>

#include <cerrno>
#if SDW_COMPILER != SDW_COMPILER_MSC || (SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION >= 1800)
#include <cinttypes>
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
#include <clocale>
#include <cstdarg>
#if SDW_COMPILER != SDW_COMPILER_MSC || (SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION >= 1600)
#include <cstdint>
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#ifndef UINT32_MAX
#define UINT32_MAX       0xffffffffU
#endif
#ifndef UINT32_C
#define UINT32_C(x)  (x ## U)
#endif
#ifndef UINT64_C
#define UINT64_C(x)  (x ## ULL)
#endif
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <bitset>
#if SDW_COMPILER == SDW_COMPILER_CLANG || (SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION >= 1600) || (SDW_COMPILER == SDW_COMPILER_GNUC && SDW_COMPILER_VERSION >= 50400)
#include <codecvt>
#endif
#include <list>
#include <locale>
#include <map>
#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
#if SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION < 1600
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

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#if SDW_COMPILER == SDW_COMPILER_MSC
#if SDW_COMPILER_VERSION < 1600
#define nullptr NULL
#endif
#if SDW_COMPILER_VERSION < 1600
typedef wchar_t Char16_t;
typedef wstring U16String;
#elif SDW_COMPILER_VERSION >= 1900
typedef u16 Char16_t;
typedef basic_string<Char16_t> U16String;
#else
typedef char16_t Char16_t;
typedef u16string U16String;
#endif
#else
typedef wchar_t Char16_t;
typedef wstring U16String;
#endif
typedef wchar_t UChar;
typedef wstring UString;
typedef wregex URegex;
typedef struct _stat64 Stat;
#else
typedef char16_t Char16_t;
typedef u16string U16String;
typedef char UChar;
typedef string UString;
typedef regex URegex;
typedef struct stat Stat;
#endif

n64 Align(n64 a_nData, n64 a_nAlignment);

#define SDW_PURE = 0

#define SDW_ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

#define SDW_BIT32(n) (UINT32_C(1) << (n))

#define SDW_BIT64(n) (UINT64_C(1) << (n))

#define SDW_CONVERT_ENDIAN32(n) (((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | ((n) << 8 & 0xFF0000) | ((n) << 24 & 0xFF000000))

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define Mkdir _mkdir
#define UMkdir _wmkdir
#else
#define Mkdir(x) mkdir((x), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define UMkdir Mkdir
#endif

bool UMakeDir(const UChar* a_pDirName);

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define Chsize _chsize_s
#define Fileno _fileno
#define UFopen FopenW
#define Fseek _fseeki64
#define Ftell _ftelli64
#define Lseek _lseeki64
#else
#define Chsize ftruncate
#define Fileno fileno
#define UFopen Fopen
#define Fseek fseeko
#define Ftell ftello
#define Lseek lseek
#endif

bool UGetFileSize(const UChar* a_pFileName, n64& a_nFileSize);

FILE* Fopen(const char* a_pFileName, const char* a_pMode, bool a_bVerbose = true);

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
FILE* FopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode, bool a_bVerbose = true);
#endif

bool Seek(FILE* a_fpFile, n64 a_nOffset);

void CopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize);

void PadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData);

#if !defined(SDW_MAIN)
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define UMain wmain
#else
#define UMain main
#endif
#endif

const UString& UGetModuleFileName();

const UString& UGetModuleDirName();

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define USTR(x) L##x
#define PRIUS USTR("ls")
#define UCscmp wcscmp
#define UCslen wcslen
#define UPrintf wprintf
#define UStat _wstat64
#else
#define USTR(x) x
#define PRIUS USTR("s")
#define UCscmp strcmp
#define UCslen strlen
#define UPrintf printf
#define UStat stat
#endif

void SetLocale();

n32 SToN32(const string& a_sString, int a_nRadix = 10);
n32 SToN32(const wstring& a_sString, int a_nRadix = 10);

#if (SDW_COMPILER == SDW_COMPILER_GNUC && SDW_COMPILER_VERSION < 50400) || SDW_PLATFORM == SDW_PLATFORM_CYGWIN || (SDW_PLATFORM != SDW_PLATFORM_WINDOWS && defined(SDW_XCONVERT))
template<typename TSrc, typename TDest>
TDest TSToS(const TSrc& a_sString, const string& a_sSrcType, const string& a_sDestType)
{
	TDest sConverted;
	iconv_t cd = iconv_open(a_sDestType.c_str(), a_sSrcType.c_str());
	if (cd == reinterpret_cast<iconv_t>(-1))
	{
		return sConverted;
	}
	size_t uStringLeft = a_sString.size() * sizeof(typename TSrc::value_type);
	static const int c_nBufferSize = 1024;
	static const int c_nConvertBufferSize = c_nBufferSize - 4;
	char szBuffer[c_nBufferSize];
	typename TSrc::value_type* pString = const_cast<typename TSrc::value_type*>(a_sString.c_str());
	do
	{
		char* pBuffer = szBuffer;
		size_t uBufferLeft = c_nConvertBufferSize;
		int nError = static_cast<int>(iconv(cd, reinterpret_cast<char**>(&pString), &uStringLeft, &pBuffer, &uBufferLeft));
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
		if (nError >= 0 || (nError == -1 && errno == E2BIG))
#else
		if (nError == 0 || (nError == -1 && errno == E2BIG))
#endif
		{
			*reinterpret_cast<typename TDest::value_type*>(szBuffer + c_nConvertBufferSize - uBufferLeft) = 0;
			sConverted += reinterpret_cast<typename TDest::value_type*>(szBuffer);
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
	iconv_close(cd);
	return sConverted;
}
#endif

string WToU8(const wstring& a_sString);
string U16ToU8(const U16String& a_sString);
wstring U8ToW(const string& a_sString);
wstring U16ToW(const U16String& a_sString);
U16String U8ToU16(const string& a_sString);
U16String WToU16(const wstring& a_sString);
wstring AToW(const string& a_sString);
string WToA(const wstring& a_sString);

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
#define U16ToU(x) U16ToW(x)
#define UToU16(x) WToU16(x)
#define AToU(x) AToW(x)
#define UToA(x) WToA(x)
#else
#define U16ToU(x) U16ToU8(x)
#define UToU16(x) U8ToU16(x)
#define AToU(x) string(x)
#define UToA(x) string(x)
#endif

string FormatV(const char* a_szFormat, va_list a_vaList);
wstring FormatV(const wchar_t* a_szFormat, va_list a_vaList);
string Format(const char* a_szFormat, ...);
wstring Format(const wchar_t* a_szFormat, ...);

template<typename T>
T Replace(const T& a_sString, typename T::value_type a_cSubChar, typename T::value_type a_cReplacement)
{
	T sString = a_sString;
	replace(sString.begin(), sString.end(), a_cSubChar, a_cReplacement);
	return sString;
}

template<typename T>
vector<T> Split(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	if (a_sSeparator.size() != 0)
	{
		for (typename T::size_type uOffset = 0; uOffset <= a_sString.size(); uOffset += a_sSeparator.size())
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
	}
	else
	{
		vString.push_back(a_sString);
	}
	return vString;
}

template<typename T>
vector<T> Split(const T& a_sString, const typename T::value_type* a_pSeparator)
{
	if (a_pSeparator == nullptr)
	{
		vector<T> vString;
		vString.push_back(a_sString);
		return vString;
	}
	else
	{
		return Split(a_sString, T(a_pSeparator));
	}
}

template<typename T>
vector<T> SplitOf(const T& a_sString, const T& a_sSeparatorSet)
{
	vector<T> vString;
	if (a_sSeparatorSet.size() != 0)
	{
		for (typename T::size_type uOffset = 0; uOffset <= a_sString.size(); uOffset++)
		{
			typename T::size_type uPos = a_sString.find_first_of(a_sSeparatorSet, uOffset);
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
	}
	else
	{
		vString.push_back(a_sString);
	}
	return vString;
}

template<typename T>
vector<T> SplitOf(const T& a_sString, const typename T::value_type* a_pSeparatorSet)
{
	if (a_pSeparatorSet == nullptr)
	{
		vector<T> vString;
		vString.push_back(a_sString);
		return vString;
	}
	else
	{
		return SplitOf(a_sString, T(a_pSeparatorSet));
	}
}

template<typename T>
bool StartWith(const T& a_sString, const T& a_sPrefix, typename T::size_type a_uStart = 0)
{
	if (a_uStart > a_sString.size())
	{
		return false;
	}
	else
	{
		return a_sString.compare(a_uStart, a_sPrefix.size(), a_sPrefix) == 0;
	}
}

template<typename T>
bool StartWith(const T& a_sString, const typename T::value_type* a_pPrefix, typename T::size_type a_uStart = 0)
{
	if (a_pPrefix == nullptr)
	{
		return false;
	}
	else
	{
		return StartWith(a_sString, T(a_pPrefix), a_uStart);
	}
}

template<typename T>
bool EndWith(const T& a_sString, const T& a_sSuffix)
{
	if (a_sString.size() < a_sSuffix.size())
	{
		return false;
	}
	else
	{
		return a_sString.compare(a_sString.size() - a_sSuffix.size(), a_sSuffix.size(), a_sSuffix) == 0;
	}
}

template<typename T>
bool EndWith(const T& a_sString, const typename T::value_type* a_pSuffix)
{
	if (a_pSuffix == nullptr)
	{
		return false;
	}
	else
	{
		return EndWith(a_sString, T(a_pSuffix));
	}
}

template<typename T>
T Trim(const T& a_sString)
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
	if (uPos > 0 || uSize < a_sString.size())
	{
		return a_sString.substr(uPos, uSize - uPos);
	}
	else
	{
		return a_sString;
	}
}

#endif	// UTILITY_H_
