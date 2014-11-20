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
#else
#include <dirent.h>
#endif
#include <errno.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _3DSTOOL_COMPILER == COMPILER_MSC
#include <io.h>
#else
#include <iconv.h>
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <algorithm>
#include <map>
#include <stack>
#include <string>
#include <vector>
#if _3DSTOOL_COMPILER == COMPILER_MSC
#include <codecvt>
#endif

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
#define FPrintf wprintf
#define MSC_PUSH_PACKED <pshpack1.h>
#define MSC_POP_PACKED <poppack.h>
#define GNUC_PACKED
#else
typedef string String;
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
#define FPrintf printf
#define MSC_PUSH_PACKED <stdlib.h>
#define MSC_POP_PACKED <stdlib.h>
#define GNUC_PACKED __attribute__((packed))
#endif

#define CONVERT_ENDIAN(n) ((n) >> 24 & 0xFF) | ((n) >> 8 & 0xFF00) | (((n) & 0xFF00) << 8) | (((n) & 0xFF) << 24)

void FSetLocale();

n32 FSToN32(const string& a_sString);

template<typename T>
vector<T> FSSplit(const T& a_sString, const T& a_sSeparator)
{
	vector<T> vString;
	for (typename T::size_type stOffset = 0; stOffset < a_sString.size(); stOffset += a_sSeparator.size())
	{
		typename T::size_type stPos = a_sString.find(a_sSeparator, stOffset);
		if (stPos != T::npos)
		{
			vString.push_back(a_sString.substr(stOffset, stPos - stOffset));
			stOffset = stPos;
		}
		else
		{
			vString.push_back(a_sString.substr(stOffset));
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

void FCopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize);

void FPadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData);

bool FCryptoFile(const char* a_pDataFileName, const char* a_pXorFileName, n64 a_nDataOffset, n64 a_nDataSize, bool a_bDataFileAll, n64 a_nXorOffset, bool a_bVerbose);

bool FGetFileSize(const String::value_type* a_pFileName, n64& a_nFileSize);

bool FMakeDir(const String::value_type* a_pDirName);

FILE* FFopenA(const char* a_pFileName, const char* a_pMode);

#if _3DSTOOL_COMPILER == COMPILER_MSC
FILE* FFopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode);
#endif

bool FSeek(FILE* a_fpFile, n64 a_nOffset);

n64 FAlign(n64 a_nOffset, n64 a_nAlignment);

size_t FU16Strlen(const char16_t* a_pString);

#endif	// UTILITY_H_
