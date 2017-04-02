#ifndef NCCH_H_
#define NCCH_H_

#include "utility.h"
#include "3dstool.h"

#include SDW_MSC_PUSH_PACKED
struct NcchCommonHeaderStruct
{
	u32 Signature;
	u32 ContentSize;
	u64 PartitionId;
	u16 MakerCode;
	u16 NcchVersion;
	u8 Reserved0[4];
	u64 ProgramId;
	u8 Reserved1[16];
	u8 LogoRegionHash[32];
	u8 ProductCode[16];
	u8 ExtendedHeaderHash[32];
	u32 ExtendedHeaderSize;
	u8 Reserved2[4];
	u8 Flags[8];
	u32 PlainRegionOffset;
	u32 PlainRegionSize;
	u32 LogoRegionOffset;
	u32 LogoRegionSize;
	u32 ExeFsOffset;
	u32 ExeFsSize;
	u32 ExeFsHashRegionSize;
	u8 Reserved4[4];
	u32 RomFsOffset;
	u32 RomFsSize;
	u32 RomFsHashRegionSize;
	u8 Reserved5[4];
	u8 ExeFsSuperBlockHash[32];
	u8 RomFsSuperBlockHash[32];
} SDW_GNUC_PACKED;

struct SNcchHeader
{
	u8 RSASignature[256];
	NcchCommonHeaderStruct Ncch;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CNcch
{
public:
	enum Index
	{
		Encrypt7x = 3,
		ContentPlatform = 4,
		ContentType,
		SizeType,
		Flag
	};
	enum Flag
	{
		FixedCryptoKey,
		NoMountRomFs,
		NoEncrypto
	};
	enum FormType
	{
		NotAssign,
		SimpleContent,
		ExecutableContentWithoutRomFs,
		ExecutableContent
	};
	enum EEncryptMode
	{
		kEncryptModeNone,
		kEncryptModeAesCtr,
		kEncryptModeXor
	};
	enum EAesCtrType
	{
		kAesCtrTypeExtendedHeader = 1,
		kAesCtrTypeExeFs,
		kAesCtrTypeRomFs
	};
	enum EOffsetSizeIndex
	{
		kOffsetSizeIndexExtendedHeader,
		kOffsetSizeIndexLogoRegion,
		kOffsetSizeIndexPlainRegion,
		kOffsetSizeIndexExeFs,
		kOffsetSizeIndexRomFs,
		kOffsetSizeIndexCount
	};
	CNcch();
	~CNcch();
	void SetFileType(C3dsTool::EFileType a_eFileType);
	void SetFileName(const string& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const string& a_sHeaderFileName);
	void SetEncryptMode(int a_nEncryptMode);
	void SetKey(const CBigNum& a_Key);
	void SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash);
	void SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash);
	void SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash);
	void SetExtendedHeaderFileName(const string& a_sExtendedHeaderFileName);
	void SetLogoRegionFileName(const string& a_sLogoRegionFileName);
	void SetPlainRegionFileName(const string& a_sPlainRegionFileName);
	void SetExeFsFileName(const string& a_sExeFsFileName);
	void SetRomFsFileName(const string& a_sRomFsFileName);
	void SetExtendedHeaderXorFileName(const string& a_sExtendedHeaderXorFileName);
	void SetExeFsXorFileName(const string& a_sExeFsXorFileName);
	void SetExeFsTopXorFileName(const string& a_sExeFsTopXorFileName);
	void SetRomFsXorFileName(const string& a_sRomFsXorFileName);
	void SetExeFsTopAutoKey(bool a_bExeFsTopAutoKey);
	void SetRomFsAutoKey(bool a_bRomFsAutoKey);
	void SetFilePtr(FILE* a_fpNcch);
	void SetOffset(n64 a_nOffset);
	SNcchHeader& GetNcchHeader();
	n64* GetOffsetAndSize();
	bool ExtractFile();
	bool CreateFile();
	bool EncryptFile();
	void Analyze();
	static bool IsCxiFile(const string& a_sFileName);
	static bool IsCfaFile(const string& a_sFileName);
	static const u32 s_uSignature;
	static const int s_nBlockSize;
private:
	string& getExtKey();
	void calculateMediaUnitSize();
	void calculateOffsetSize();
	void calculateAlignment();
	void calculateKey();
	void calculateCounter(EAesCtrType a_eAesCtrType);
	bool extractFile(const string& a_sFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const string& a_sType);
	bool createHeader();
	bool createExtendedHeader();
	bool createLogoRegion();
	bool createPlainRegion();
	bool createExeFs();
	bool createRomFs();
	void clearExtendedHeader();
	void clearLogoRegion();
	void clearPlainRegion();
	void clearExeFs();
	void clearRomFs();
	void alignFileSize(n64 a_nAlignment);
	bool encryptAesCtrFile(n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const string& a_sType);
	bool encryptXorFile(const string& a_sXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const string& a_sType);
	static size_t onDownload(char* a_pData, size_t a_uSize, size_t a_uNmemb, void* a_pUserData);
	static const CBigNum s_Slot0x18KeyX;
	static const CBigNum s_Slot0x1BKeyX;
	static const CBigNum s_Slot0x25KeyX;
	C3dsTool::EFileType m_eFileType;
	string m_sFileName;
	bool m_bVerbose;
	string m_sHeaderFileName;
	int m_nEncryptMode;
	CBigNum m_Key;
	bool m_bNotUpdateExtendedHeaderHash;
	bool m_bNotUpdateExeFsHash;
	bool m_bNotUpdateRomFsHash;
	string m_sExtendedHeaderFileName;
	string m_sLogoRegionFileName;
	string m_sPlainRegionFileName;
	string m_sExeFsFileName;
	string m_sRomFsFileName;
	string m_sExtendedHeaderXorFileName;
	string m_sExeFsXorFileName;
	string m_sExeFsTopXorFileName;
	string m_sRomFsXorFileName;
	bool m_bExeFsTopAutoKey;
	bool m_bRomFsAutoKey;
	FILE* m_fpNcch;
	n64 m_nOffset;
	SNcchHeader m_NcchHeader;
	n64 m_nMediaUnitSize;
	n64 m_nOffsetAndSize[kOffsetSizeIndexCount * 2];
	bool m_bAlignToBlockSize;
	string m_sExtKey;
	CBigNum m_Counter;
	string m_sXorFileName;
};

#endif	// NCCH_H_
