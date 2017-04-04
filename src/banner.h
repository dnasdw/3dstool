#ifndef BANNER_H_
#define BANNER_H_

#include "utility.h"

#include SDW_MSC_PUSH_PACKED
struct SCbmdHeader
{
	u32 Signature;
	u32 CbmdOffset;
	u32 CgfxOffset[31];
	u32 CwavOffset;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CBanner
{
public:
	CBanner();
	~CBanner();
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetBannerDirName(const UString& a_sBannerDirName);
	bool ExtractFile();
	bool CreateFile();
	static bool IsBannerFile(const UString& a_sFileName);
	static const u32 s_uSignature;
	static const int s_nCbmdSizeAlignment;
	static const UChar* s_pCbmdHeaderFileName;
	static const UChar* s_pCbmdBodyFileName;
	static const UChar* s_pBcwavFileName;
private:
	bool extractCbmdHeader();
	bool extractCbmdBody();
	bool extractBcwav();
	bool createCbmdHeader();
	bool createCbmdBody();
	bool createBcwav();
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sBannerDirName;
	FILE* m_fpBanner;
	SCbmdHeader m_CbmdHeader;
};

#endif	// BANNER_H_
