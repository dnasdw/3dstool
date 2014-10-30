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
	u32 Level0Size;
	SRomFsLevel Level1;
	SRomFsLevel Level2;
	SRomFsLevel Level3;
	u32 Size;
	u32 Reserved;
} GNUC_PACKED;

struct SRomFsMetaInfoSection
{
	u32 Offset;
	u32 Size;
} GNUC_PACKED;

struct SRomFsMetaInfo
{
	u32 Size;
	SRomFsMetaInfoSection Section[kSectionTypeCount];
	u32 DataOffset;
} GNUC_PACKED;

struct SCommonDirEntry
{
	n32 ParentDirOffset;
	n32 SiblingDirOffset;
	n32 ChildDirOffset;
	n32 ChildFileOffset;
	n32 PrevDirOffset;
	n32 NameSize;
} GNUC_PACKED;

struct SCommonFileEntry
{
	n32 ParentDirOffset;
	n32 SiblingFileOffset;
	n64 FileOffset;
	n64 FileSize;
	n32 PrevFileOffset;
	n32 NameSize;
} GNUC_PACKED;
#include MSC_POP_PACKED

union UCommonEntry
{
	SCommonDirEntry Dir;
	SCommonFileEntry File;
};

enum EExtractState
{
	kExtractStateBegin,
	kExtractStateChildDir,
	kExtractStateSiblingDir,
	kExtractStateEnd
};

struct SExtractStackElement
{
	bool IsDir;
	n64 EntryOffset;
	UCommonEntry Entry;
	String EntryName;
	String Prefix;
	EExtractState ExtractState;
};

struct SEntry
{
	String Path;
	u16string EntryName;
	int EntryNameSize;
	n32 EntryOffset;
	n32 BucketIndex;
	UCommonEntry Entry;
};

struct SCreateStackElement
{
	int EntryOffset;
	vector<int> ChildOffset;
	int ChildIndex;
};

struct SLevelBuffer
{
	vector<u8> Data;
	int DataPos;
	n64 FilePos;
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
	bool CreateFile();
	static bool IsRomFsFile(const char* a_pFileName);
	static const u32 s_uSignature = CONVERT_ENDIAN('IVFC');
	static const int s_nBlockSizePower = 0xC;
	static const int s_nBlockSize = 1 << s_nBlockSizePower;
	static const int s_nSHA256BlockSize = 0x20;
	static const n32 s_nInvalidOffset = -1;
	static const int s_nEntryNameAlignment = 4;
	static const n64 s_nFileSizeAlignment = 0x10;
private:
	bool extractDirEntry();
	bool extractFileEntry();
	void readEntry(SExtractStackElement& a_Element);
	void pushExtractStackElement(bool a_bIsDir, n64 a_nEntryOffset, const String& a_sPrefix);
	void setupCreate();
	void pushDirEntry(const String& a_sEntryName, n32 a_nParentDirOffset);
	bool pushFileEntry(const String& a_sEntryName, n32 a_nParentDirOffset);
	void pushCreateStackElement(int a_nEntryOffset);
	bool createEntryList();
	void removeEmptyDirEntry();
	void removeDirEntry(int a_nIndex);
	void subDirOffset(n32& a_nOffset, int a_nIndex);
	void createHash();
	u32 computeBucketCount(u32 a_uEntries);
	u32 hash(n32 a_nParentOffset, u16string& a_sEntryName);
	void redirectOffset();
	void redirectOffset(n32& a_nOffset, bool a_bIsDir);
	void createMetaInfo();
	void createHeader();
	void initLevelBuffer();
	bool updateLevelBuffer();
	void writeBuffer(int a_nLevel, const void* a_pSrc, n64 a_nSize);
	bool writeBufferFromFile(int a_nLevel, const String& a_sPath, n64 a_nSize);
	void alignBuffer(int a_nLevel, int a_nAlignment);
	const char* m_pFileName;
	String m_sRomFsDirName;
	bool m_bVerbose;
	FILE* m_fpRomFs;
	SRomFsHeader m_RomFsHeader;
	n64 m_nLevel3Offset;
	SRomFsMetaInfo m_RomFsMetaInfo;
	stack<SExtractStackElement> m_sExtractStack;
	stack<SCreateStackElement> m_sCreateStack;
	vector<SEntry> m_vCreateDir;
	vector<SEntry> m_vCreateFile;
	vector<n32> m_vDirBucket;
	vector<n32> m_vFileBucket;
	SLevelBuffer m_LevelBuffer[4];
};

#endif	// ROMFS_H_
