#ifndef BANNER_H_
#define BANNER_H_

#include "utility.h"

#include MSC_PUSH_PACKED
struct SCbmdHeader
{
	u32 Signature;
	u32 Unknown4;
	u32 Offset;
	u32 Reserved[30];
	u32 FileSize;
};
#include MSC_POP_PACKED

class CBanner
{
public:
	CBanner();
	~CBanner();
	void SetFileName(const char* a_pFileName);
	void SetVerbose(bool a_bVerbose);
	void SetBannerDirName(const char* a_pBannerDirName);
	void SetUncompress(bool a_bUncompress);
	void SetCompress(bool a_bCompress);
	bool ExtractFile();
	bool CreateFile();
	static bool IsBannerFile(const char* a_pFileName);
	static const u32 s_uSignature;
	static const int s_nCbmdSizeAlignment;
	static const char* s_pCbmdHeaderFileName;
	static const char* s_pCbmdBodyFileName;
	static const char* s_pBcwavFileName;
private:
	bool extractCbmdHeader();
	bool extractCbmdBody();
	bool extractBcwav();
	bool createCbmdHeader();
	bool createCbmdBody();
	bool createBcwav();
	const char* m_pFileName;
	bool m_bVerbose;
	const char* m_pBannerDirName;
	bool m_bUncompress;
	bool m_bCompress;
	FILE* m_fpBanner;
	SCbmdHeader m_CbmdHeader;
};

#endif	// BANNER_H_
