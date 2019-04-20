#include "utility.h"

n64 Align(n64 a_nData, n64 a_nAlignment)
{
	return (a_nData + a_nAlignment - 1) / a_nAlignment * a_nAlignment;
}

bool UMakeDir(const UChar* a_pDirName)
{
	if (UMkdir(a_pDirName) != 0)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}
	return true;
}

bool UGetFileSize(const UChar* a_pFileName, n64& a_nFileSize)
{
	Stat st;
	if (UStat(a_pFileName, &st) != 0)
	{
		a_nFileSize = 0;
		return false;
	}
	a_nFileSize = st.st_size;
	return true;
}

FILE* Fopen(const char* a_pFileName, const char* a_pMode, bool a_bVerbose /* = true */)
{
	FILE* fp = fopen(a_pFileName, a_pMode);
	if (fp == nullptr && a_bVerbose)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
	}
	return fp;
}

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
FILE* FopenW(const wchar_t* a_pFileName, const wchar_t* a_pMode, bool a_bVerbose /* = true */)
{
	FILE* fp = _wfopen(a_pFileName, a_pMode);
	if (fp == nullptr && a_bVerbose)
	{
		wprintf(L"ERROR: open file %ls failed\n\n", a_pFileName);
	}
	return fp;
}
#endif

bool Seek(FILE* a_fpFile, n64 a_nOffset)
{
	if (fflush(a_fpFile) != 0)
	{
		return false;
	}
	int nFd = Fileno(a_fpFile);
	if (nFd == -1)
	{
		return false;
	}
	Fseek(a_fpFile, 0, SEEK_END);
	n64 nFileSize = Ftell(a_fpFile);
	if (nFileSize < a_nOffset)
	{
		n64 nOffset = Lseek(nFd, a_nOffset - 1, SEEK_SET);
		if (nOffset == -1)
		{
			return false;
		}
		fputc(0, a_fpFile);
		fflush(a_fpFile);
	}
	else
	{
		Fseek(a_fpFile, a_nOffset, SEEK_SET);
	}
	return true;
}

void CopyFile(FILE* a_fpDest, FILE* a_fpSrc, n64 a_nSrcOffset, n64 a_nSize)
{
	const n64 nBufferSize = 0x100000;
	u8* pBuffer = new u8[nBufferSize];
	Fseek(a_fpSrc, a_nSrcOffset, SEEK_SET);
	while (a_nSize > 0)
	{
		n64 nSize = a_nSize > nBufferSize ? nBufferSize : a_nSize;
		fread(pBuffer, 1, static_cast<size_t>(nSize), a_fpSrc);
		fwrite(pBuffer, 1, static_cast<size_t>(nSize), a_fpDest);
		a_nSize -= nSize;
	}
	delete[] pBuffer;
}

void PadFile(FILE* a_fpFile, n64 a_nPadSize, u8 a_uPadData)
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

#if defined(SDW_MAIN)
extern int UMain(int argc, UChar* argv[]);

int main(int argc, char* argv[])
{
	SetLocale();
	int nArgc = 0;
	UChar** pArgv = nullptr;
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	pArgv = CommandLineToArgvW(GetCommandLineW(), &nArgc);
	if (pArgv == nullptr)
	{
		return 1;
	}
#else
	nArgc = argc;
	pArgv = argv;
#endif
	int nResult = UMain(nArgc, pArgv);
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	LocalFree(pArgv);
#endif
	return nResult;
}
#endif

const UString& UGetModuleFileName()
{
	const u32 uMaxPath = 4096;
	static UString c_sFileName;
	if (!c_sFileName.empty())
	{
		return c_sFileName;
	}
	c_sFileName.resize(uMaxPath, USTR('\0'));
	u32 uSize = 0;
#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
	uSize = GetModuleFileNameW(nullptr, &*c_sFileName.begin(), uMaxPath);
#elif SDW_PLATFORM == SDW_PLATFORM_MACOS
	char szPath[uMaxPath] = {};
	u32 uPathSize = uMaxPath;
	if (_NSGetExecutablePath(szPath, &uPathSize) != 0)
	{
		c_sFileName.clear();
		printf("ERROR: _NSGetExecutablePath error\n\n");
	}
	else if (realpath(szPath, &*c_sFileName.begin()) == nullptr)
	{
		c_sFileName.clear();
		printf("ERROR: realpath error\n\n");
	}
	uSize = strlen(c_sFileName.c_str());
#elif SDW_PLATFORM == SDW_PLATFORM_LINUX || SDW_PLATFORM == SDW_PLATFORM_CYGWIN
	ssize_t nCount = readlink("/proc/self/exe", &*c_sFileName.begin(), uMaxPath);
	if (nCount == -1)
	{
		c_sFileName.clear();
		printf("ERROR: readlink /proc/self/exe error\n\n");
	}
	else
	{
		c_sFileName[nCount] = '\0';
	}
	uSize = strlen(c_sFileName.c_str());
#endif
	c_sFileName.erase(uSize);
	c_sFileName = Replace(c_sFileName, USTR('\\'), USTR('/'));
	return c_sFileName;
}

const UString& UGetModuleDirName()
{
	static UString c_sDirName;
	if (!c_sDirName.empty())
	{
		return c_sDirName;
	}
	c_sDirName = UGetModuleFileName();
	UString::size_type uPos = c_sDirName.rfind(USTR('/'));
	if (uPos != UString::npos)
	{
		c_sDirName.erase(uPos);
	}
	else
	{
		c_sDirName.clear();
	}
	return c_sDirName;
}

void SetLocale()
{
#if SDW_PLATFORM == SDW_PLATFORM_MACOS
	setlocale(LC_ALL, "en_US.UTF-8");
#else
	setlocale(LC_ALL, "");
#endif
}

n32 SToN32(const string& a_sString, int a_nRadix /* = 10 */)
{
	return static_cast<n32>(strtol(a_sString.c_str(), nullptr, a_nRadix));
}

n32 SToN32(const wstring& a_sString, int a_nRadix /* = 10 */)
{
	return static_cast<n32>(wcstol(a_sString.c_str(), nullptr, a_nRadix));
}

#if (SDW_COMPILER == SDW_COMPILER_MSC && SDW_COMPILER_VERSION < 1600) || (SDW_PLATFORM == SDW_PLATFORM_WINDOWS && SDW_COMPILER != SDW_COMPILER_MSC)
string WToU8(const wstring& a_sString)
{
	int nLength = WideCharToMultiByte(CP_UTF8, 0, a_sString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char* pTemp = new char[nLength];
	WideCharToMultiByte(CP_UTF8, 0, a_sString.c_str(), -1, pTemp, nLength, nullptr, nullptr);
	string sString = pTemp;
	delete[] pTemp;
	return sString;
}

string U16ToU8(const U16String& a_sString)
{
	return WToU8(a_sString);
}

wstring U8ToW(const string& a_sString)
{
	int nLength = MultiByteToWideChar(CP_UTF8, 0, a_sString.c_str(), -1, nullptr, 0);
	wchar_t* pTemp = new wchar_t[nLength];
	MultiByteToWideChar(CP_UTF8, 0, a_sString.c_str(), -1, pTemp, nLength);
	wstring sString = pTemp;
	delete[] pTemp;
	return sString;
}

wstring U16ToW(const U16String& a_sString)
{
	return a_sString;
}

U16String U8ToU16(const string& a_sString)
{
	return U8ToW(a_sString);
}

U16String WToU16(const wstring& a_sString)
{
	return a_sString;
}
#elif (SDW_COMPILER == SDW_COMPILER_GNUC && SDW_COMPILER_VERSION < 50400) || SDW_PLATFORM == SDW_PLATFORM_CYGWIN
string WToU8(const wstring& a_sString)
{
	return TSToS<wstring, string>(a_sString, "WCHAR_T", "UTF-8");
}

string U16ToU8(const U16String& a_sString)
{
	return TSToS<U16String, string>(a_sString, "UTF-16LE", "UTF-8");
}

wstring U8ToW(const string& a_sString)
{
	return TSToS<string, wstring>(a_sString, "UTF-8", "WCHAR_T");
}

wstring U16ToW(const U16String& a_sString)
{
	return TSToS<U16String, wstring>(a_sString, "UTF-16LE", "WCHAR_T");
}

U16String U8ToU16(const string& a_sString)
{
	return TSToS<string, U16String>(a_sString, "UTF-8", "UTF-16LE");
}

U16String WToU16(const wstring& a_sString)
{
	return TSToS<wstring, U16String>(a_sString, "WCHAR_T", "UTF-16LE");
}
#else
string WToU8(const wstring& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.to_bytes(a_sString);
}

string U16ToU8(const U16String& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.to_bytes(a_sString);
}

wstring U8ToW(const string& a_sString)
{
	static wstring_convert<codecvt_utf8<wchar_t>> c_cvt_u8;
	return c_cvt_u8.from_bytes(a_sString);
}

wstring U16ToW(const U16String& a_sString)
{
	return U8ToW(U16ToU8(a_sString));
}

U16String U8ToU16(const string& a_sString)
{
	static wstring_convert<codecvt_utf8_utf16<Char16_t>, Char16_t> c_cvt_u8_u16;
	return c_cvt_u8_u16.from_bytes(a_sString);
}

U16String WToU16(const wstring& a_sString)
{
	return U8ToU16(WToU8(a_sString));
}
#endif

#if SDW_PLATFORM == SDW_PLATFORM_WINDOWS
wstring AToW(const string& a_sString)
{
	int nLength = MultiByteToWideChar(CP_ACP, 0, a_sString.c_str(), -1, nullptr, 0);
	wchar_t* pTemp = new wchar_t[nLength];
	MultiByteToWideChar(CP_ACP, 0, a_sString.c_str(), -1, pTemp, nLength);
	wstring sString = pTemp;
	delete[] pTemp;
	return sString;
}

string WToA(const wstring& a_sString)
{
	int nLength = WideCharToMultiByte(CP_ACP, 0, a_sString.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char* pTemp = new char[nLength];
	WideCharToMultiByte(CP_ACP, 0, a_sString.c_str(), -1, pTemp, nLength, nullptr, nullptr);
	string sString = pTemp;
	delete[] pTemp;
	return sString;
}
#else
wstring AToW(const string& a_sString)
{
	return U8ToW(a_sString);
}

string WToA(const wstring& a_sString)
{
	return WToU8(a_sString);
}
#endif

string FormatV(const char* a_szFormat, va_list a_vaList)
{
	static const int c_nFormatBufferSize = 0x100000;
	static char c_szBuffer[c_nFormatBufferSize] = {};
	vsnprintf(c_szBuffer, c_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

wstring FormatV(const wchar_t* a_szFormat, va_list a_vaList)
{
	static const int c_nFormatBufferSize = 0x100000;
	static wchar_t c_szBuffer[c_nFormatBufferSize] = {};
	vswprintf(c_szBuffer, c_nFormatBufferSize, a_szFormat, a_vaList);
	return c_szBuffer;
}

string Format(const char* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	string sFormatted = FormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}

wstring Format(const wchar_t* a_szFormat, ...)
{
	va_list vaList;
	va_start(vaList, a_szFormat);
	wstring sFormatted = FormatV(a_szFormat, vaList);
	va_end(vaList);
	return sFormatted;
}
