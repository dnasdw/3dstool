#ifndef ROMFS_H_
#define ROMFS_H_

#include "utility.h"

enum ESectionType
{
	kSectionTypeDirHash,
	kSectionTypeDir,
	kSectionTypeFileHash,
	kSectionTypeFile,
	kSectionTypeCount
};

#include MSC_PUSH_PACKED
struct SRomFsLevel
{
	n64 LogicOffset;
	n64 Size;
	u32 BlockSize;
	u32 Reserved;
} GNUC_PACKED;

struct SRomFsHeader
{
	u32 Signature;
	u32 Id;
	u32 Level0HashSize;
	SRomFsLevel Level1;
	SRomFsLevel Level2;
	SRomFsLevel Level3;
	u32 Size;
	u32 Reserved;
} GNUC_PACKED;

struct SRomFsLevel3Section
{
	u32 Offset;
	u32 Size;
} GNUC_PACKED;

struct SRomFsLevel3Header
{
	u32 Size;
	SRomFsLevel3Section Section[kSectionTypeCount];
	u32 DataOffset;
} GNUC_PACKED;

struct SCommonDirEntry
{
	n32 ParentDirOffset;
	n32 SiblingDirOffset;
	n32 ChildDirOffset;
	n32 ChildFileOffset;
	n32 PrevDirOffset;
	n32 DirNameSize;
} GNUC_PACKED;

struct SCommonFileEntry
{
	n32 ParentDirOffset;
	n32 SiblingFileOffset;
	n64 FileOffset;
	n64 FileSize;
	n32 PrevFileOffset;
	n32 FileNameSize;
} GNUC_PACKED;
#include MSC_POP_PACKED

enum EExtractState
{
	kExtractStateBegin,
	kExtractStateChildFile,
	kExtractStateChildDir,
	kExtractStateEnd
};

struct SStackElement
{
	bool IsDir;
	n64 EntryOffset;
	union UEntry
	{
		SCommonDirEntry Dir;
		SCommonFileEntry File;
	} Entry;
#if _3DSTOOL_COMPILER == COMPILER_MSC
	wstring EntryName;
	wstring Prefix;
#else
	string EntryName;
	string Prefix;
#endif
	EExtractState ExtractState;
};

class CRomFs
{
public:
	CRomFs();
	~CRomFs();
	void SetFileName(const char* a_pFileName);
	void SetRomFsDirName(const char* a_pRomFsDirName);
	void SetVerbose(bool a_bVerbose);
	bool ExtractFile();
	bool ExtractDirEntry();
	bool ExtractFileEntry();
	static bool IsRomFsFile(const char* a_pFileName);
	static const u32 s_uSignature = CONVERT_ENDIAN('IVFC');
	static const int s_nLevel0BlockSize = 0x1000;
	static const n32 s_nInvalidOffset = -1;
private:
	const char* m_pFileName;
#if _3DSTOOL_COMPILER == COMPILER_MSC
	wstring m_sRomFsDirName;
#else
	string m_sRomFsDirName;
#endif
	bool m_bVerbose;
	FILE* m_fpRomFsBin;
	SRomFsHeader m_RomFsHeader;
	n64 m_nLevel3Offset;
	SRomFsLevel3Header m_RomFsLevel3Header;
	stack<SStackElement> m_sExtractStack;
};

#endif	// ROMFS_H_
