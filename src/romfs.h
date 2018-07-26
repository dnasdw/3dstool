#ifndef ROMFS_H_
#define ROMFS_H_

#include "utility.h"

#include SDW_MSC_PUSH_PACKED
struct SRomFsLevel
{
	n64 LogicOffset;
	n64 Size;
	u32 BlockSize;
	u32 Reserved;
} SDW_GNUC_PACKED;

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
} SDW_GNUC_PACKED;

struct SRomFsMetaInfoSection
{
	u32 Offset;
	u32 Size;
} SDW_GNUC_PACKED;

struct SRomFsMetaInfo
{
	u32 Size;
	SRomFsMetaInfoSection Section[4];
	u32 DataOffset;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CRomFs
{
public:
	enum ESectionType
	{
		kSectionTypeDirHash,
		kSectionTypeDir,
		kSectionTypeFileHash,
		kSectionTypeFile
	};
	enum EExtractState
	{
		kExtractStateBegin,
		kExtractStateChildDir,
		kExtractStateSiblingDir,
		kExtractStateEnd
	};
	struct SCommonDirEntry
	{
		n32 ParentDirOffset;
		n32 SiblingDirOffset;
		n32 ChildDirOffset;
		n32 ChildFileOffset;
		n32 PrevDirOffset;
		n32 NameSize;
	};
	struct SCommonFileEntry
	{
		n32 ParentDirOffset;
		n32 SiblingFileOffset;
		union
		{
			n64 FileOffset;
			u64 RemapIgnoreLevel;
		};
		n64 FileSize;
		n32 PrevFileOffset;
		n32 NameSize;
	};
	union UCommonEntry
	{
		SCommonDirEntry Dir;
		SCommonFileEntry File;
	};
	struct SExtractStackElement
	{
		bool IsDir;
		n32 EntryOffset;
		UCommonEntry Entry;
		UString EntryName;
		UString Prefix;
		EExtractState ExtractState;
	};
	struct SEntry
	{
		UString Path;
		U16String EntryName;
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
		SLevelBuffer()
			: DataPos(0)
			, FilePos(0)
		{
		}
	};
	CRomFs();
	~CRomFs();
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetRomFsDirName(const UString& a_sRomFsDirName);
	void SetRomFsFileName(const UString& a_sRomFsFileName);
	bool ExtractFile();
	bool CreateFile();
	static bool IsRomFsFile(const UString& a_sFileName);
	static const u32 s_uSignature;
	static const int s_nBlockSizePower;
	static const int s_nBlockSize;
	static const int s_nSHA256BlockSize;
	static const n32 s_nInvalidOffset;
	static const int s_nEntryNameAlignment;
	static const n64 s_nFileSizeAlignment;
private:
	void pushExtractStackElement(bool a_bIsDir, n32 a_nEntryOffset, const UString& a_sPrefix);
	bool extractDirEntry();
	bool extractFileEntry();
	void readEntry(SExtractStackElement& a_Element);
	void setupCreate();
	void buildIgnoreList();
	void pushDirEntry(const UString& a_sEntryName, n32 a_nParentDirOffset);
	bool pushFileEntry(const UString& a_sEntryName, n32 a_nParentDirOffset);
	void pushCreateStackElement(int a_nEntryOffset);
	bool createEntryList();
	bool matchInIgnoreList(const UString& a_sPath) const;
	u32 getRemapIgnoreLevel(const UString& a_sPath) const;
	void removeEmptyDirEntry();
	void removeDirEntry(int a_nIndex);
	void subDirOffset(n32& a_nOffset, int a_nIndex);
	void createHash();
	u32 computeBucketCount(u32 a_uEntries);
	void redirectOffset();
	void redirectOffset(n32& a_nOffset, bool a_bIsDir);
	void createMetaInfo();
	void remap();
	bool travelFile();
	void travelDirEntry();
	void travelFileEntry();
	void createHeader();
	void initLevelBuffer();
	bool updateLevelBuffer();
	void writeBuffer(int a_nLevel, const void* a_pSrc, n64 a_nSize);
	bool writeBufferFromFile(int a_nLevel, const UString& a_sPath, n64 a_nSize);
	void alignBuffer(int a_nLevel, int a_nAlignment);
	static u32 hash(n32 a_nParentOffset, U16String& a_sEntryName);
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sRomFsDirName;
	UString m_sRomFsFileName;
	FILE* m_fpRomFs;
	SRomFsHeader m_RomFsHeader;
	n64 m_nLevel3Offset;
	SRomFsMetaInfo m_RomFsMetaInfo;
	stack<SExtractStackElement> m_sExtractStack;
	vector<URegex> m_vIgnoreList;
	vector<URegex> m_vRemapIgnoreList;
	vector<SEntry> m_vCreateDir;
	vector<SEntry> m_vCreateFile;
	stack<SCreateStackElement> m_sCreateStack;
	vector<n32> m_vDirBucket;
	vector<n32> m_vFileBucket;
	map<UString, SCommonFileEntry> m_mTravelInfo;
	bool m_bRemapped;
	SLevelBuffer m_LevelBuffer[4];
};

#endif	// ROMFS_H_
