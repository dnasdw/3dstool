#ifndef EXEFS_H_
#define EXEFS_H_

#include "utility.h"

#include SDW_MSC_PUSH_PACKED
struct ExeSectionHeaderStruct
{
	u8 name[8];
	u32 offset;
	u32 size;
} SDW_GNUC_PACKED;

struct ExeFsSuperBlock
{
	ExeSectionHeaderStruct m_Header[8];
	u8 m_Reserved[128];
	u8 m_Hash[8][32];
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CExeFs
{
public:
	CExeFs();
	~CExeFs();
	void SetFileName(const char* a_pFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const string& a_sHeaderFileName);
	void SetExeFsDirName(const string& a_sExeFsDirName);
	void SetUncompress(bool a_bUncompress);
	void SetCompress(bool a_bCompress);
	bool ExtractFile();
	bool CreateFile();
	static bool IsExeFsFile(const char* a_pFileName, n64 a_nOffset);
	static bool IsExeFsSuperBlock(const ExeFsSuperBlock& a_ExeFsSuperBlock);
	static const int s_nBlockSize;
private:
	bool extractHeader();
	bool extractSection(int a_nIndex);
	bool createHeader();
	bool createSection(int a_nIndex);
	void clearSection(int a_nIndex);
	const char* m_pFileName;
	bool m_bVerbose;
	string m_sHeaderFileName;
	UString m_sExeFsDirName;
	bool m_bUncompress;
	bool m_bCompress;
	FILE* m_fpExeFs;
	ExeFsSuperBlock m_ExeFsSuperBlock;
	unordered_map<string, UString> m_mPath;
};

#endif	// EXEFS_H_
