#ifndef NCCH_H_
#define NCCH_H_

#include "utility.h"
#include "3dstool.h"

class CUrl;

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
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const UString& a_sHeaderFileName);
	void SetEncryptMode(int a_nEncryptMode);
	void SetKey(const CBigNum& a_Key);
	void SetDownloadBegin(n32 a_nDownloadBegin);
	void SetDownloadEnd(n32 a_nDownloadEnd);
	void SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash);
	void SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash);
	void SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash);
	void SetExtendedHeaderFileName(const UString& a_sExtendedHeaderFileName);
	void SetLogoRegionFileName(const UString& a_sLogoRegionFileName);
	void SetPlainRegionFileName(const UString& a_sPlainRegionFileName);
	void SetExeFsFileName(const UString& a_sExeFsFileName);
	void SetRomFsFileName(const UString& a_sRomFsFileName);
	void SetExtendedHeaderXorFileName(const UString& a_sExtendedHeaderXorFileName);
	void SetExeFsXorFileName(const UString& a_sExeFsXorFileName);
	void SetExeFsTopXorFileName(const UString& a_sExeFsTopXorFileName);
	void SetRomFsXorFileName(const UString& a_sRomFsXorFileName);
	void SetExtendedHeaderAutoKey(bool a_bExtendedHeaderAutoKey);
	void SetExeFsAutoKey(bool a_bExeFsAutoKey);
	void SetExeFsTopAutoKey(bool a_bExeFsTopAutoKey);
	void SetRomFsAutoKey(bool a_bRomFsAutoKey);
	void SetFilePtr(FILE* a_fpNcch);
	void SetOffset(n64 a_nOffset);
	SNcchHeader& GetNcchHeader();
	n64* GetOffsetAndSize();
	bool ExtractFile();
	bool CreateFile();
	bool EncryptFile();
	bool Download(bool a_bReadExtKey = true);
	void Analyze();
	static bool IsCxiFile(const UString& a_sFileName);
	static bool IsCfaFile(const UString& a_sFileName);
	static const u32 s_uSignature;
	static const int s_nBlockSize;
private:
	void calculateMediaUnitSize();
	void calculateOffsetSize();
	void calculateAlignment();
	void calculateKey();
	void readExtKey();
	bool writeExtKey();
	void calculateCounter(EAesCtrType a_eAesCtrType);
	bool extractFile(const UString& a_sFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const UChar* a_pType);
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
	bool encryptAesCtrFile(n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const UChar* a_pType);
	bool encryptXorFile(const UString& a_sXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const UChar* a_pType);
	void onHttpsGetExtKey(CUrl* a_pUrl, void* a_pUserData);
	static const CBigNum s_Slot0x18KeyX;
	static const CBigNum s_Slot0x1BKeyX;
	static const CBigNum s_Slot0x25KeyX;
	static const CBigNum s_Slot0x2CKeyX;
	C3dsTool::EFileType m_eFileType;
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sHeaderFileName;
	int m_nEncryptMode;
	CBigNum m_Key[2];
	n32 m_nDownloadBegin;
	n32 m_nDownloadEnd;
	bool m_bNotUpdateExtendedHeaderHash;
	bool m_bNotUpdateExeFsHash;
	bool m_bNotUpdateRomFsHash;
	UString m_sExtendedHeaderFileName;
	UString m_sLogoRegionFileName;
	UString m_sPlainRegionFileName;
	UString m_sExeFsFileName;
	UString m_sRomFsFileName;
	UString m_sExtendedHeaderXorFileName;
	UString m_sExeFsXorFileName;
	UString m_sExeFsTopXorFileName;
	UString m_sRomFsXorFileName;
	bool m_bExtendedHeaderAutoKey;
	bool m_bExeFsAutoKey;
	bool m_bExeFsTopAutoKey;
	bool m_bRomFsAutoKey;
	FILE* m_fpNcch;
	n64 m_nOffset;
	SNcchHeader m_NcchHeader;
	n64 m_nMediaUnitSize;
	n64 m_nOffsetAndSize[kOffsetSizeIndexCount * 2];
	bool m_bAlignToBlockSize;
	map<string, string> m_mExtKey;
	int m_nKeyIndex;
	CBigNum m_Counter;
	UString m_sXorFileName;
};

#endif	// NCCH_H_
