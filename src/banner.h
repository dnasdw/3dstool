#ifndef BANNER_H_
#define BANNER_H_

#include "utility.h"

#include MSC_PUSH_PACKED
struct SCbmdHeader
{
	u32 Signature;
	u32 CbmdOffset;
	u32 CgfxOffset[31];
	u32 CwavOffset;
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
	FILE* m_fpBanner;
	SCbmdHeader m_CbmdHeader;
};

#endif	// BANNER_H_
