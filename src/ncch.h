#ifndef NCCH_H_
#define NCCH_H_

#include "utility.h"

#include MSC_PUSH_PACKED
struct NcchCommonHeaderStruct
{
	u32 Signature;
	u32 ContentSize;
	u64 PartitionId;
	u16 MakerCode;
	u16 NcchVersion;
	u8 Reserved0[4];
	u64 ProgramId;
	u8 TempFlag;
	u8 Reserved1[15];
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
} GNUC_PACKED;

struct SNcchHeader
{
	u8 RSASignature[256];
	NcchCommonHeaderStruct Ncch;
} GNUC_PACKED;
#include MSC_POP_PACKED

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
	CNcch();
	~CNcch();
	void SetFileName(const char* a_pFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const char* a_pHeaderFileName);
	void SetEncryptMode(int a_nEncryptMode);
	void SetKey(u8 a_uKey[16]);
	void SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash);
	void SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash);
	void SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash);
	void SetExtendedHeaderFileName(const char* a_pExtendedHeaderFileName);
	void SetLogoRegionFileName(const char* a_pLogoRegionFileName);
	void SetPlainRegionFileName(const char* a_pPlainRegionFileName);
	void SetExeFsFileName(const char* a_pExeFsFileName);
	void SetRomFsFileName(const char* a_pRomFsFileName);
	void SetExtendedHeaderXorFileName(const char* a_pExtendedHeaderXorFileName);
	void SetExeFsXorFileName(const char* a_pExeFsXorFileName);
	void SetExeFsTopXorFileName(const char* a_pExeFsTopXorFileName);
	void SetRomFsXorFileName(const char* a_pRomFsXorFileName);
	bool ExtractFile();
	bool CreateFile();
	bool EncryptFile();
	static bool IsCxiFile(const char* a_pFileName);
	static bool IsCfaFile(const char* a_pFileName);
	static const u32 s_uSignature;
	static const int s_nBlockSize;
private:
	void calculateMediaUnitSize();
	void calculateOffsetSize();
	void calculateAlignment();
	bool extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, bool a_bPlainData, const char* a_pType);
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
	bool encryptAesCtrFile(n64 a_nOffset, n64 a_nSize, const char* a_pType);
	bool encryptXorFile(const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType);
	static void getAesCounter(NcchCommonHeaderStruct* a_pNcch, EAesCtrType a_eAesCtrType, n64 a_nMediaUnitSize, u8 a_uAesCtr[16]);
	const char* m_pFileName;
	bool m_bVerbose;
	const char* m_pHeaderFileName;
	int m_nEncryptMode;
	u8 m_uKey[16];
	bool m_bNotUpdateExtendedHeaderHash;
	bool m_bNotUpdateExeFsHash;
	bool m_bNotUpdateRomFsHash;
	const char* m_pExtendedHeaderFileName;
	const char* m_pLogoRegionFileName;
	const char* m_pPlainRegionFileName;
	const char* m_pExeFsFileName;
	const char* m_pRomFsFileName;
	const char* m_pExtendedHeaderXorFileName;
	const char* m_pExeFsXorFileName;
	const char* m_pExeFsTopXorFileName;
	const char* m_pRomFsXorFileName;
	FILE* m_fpNcch;
	SNcchHeader m_NcchHeader;
	n64 m_nMediaUnitSize;
	n64 m_nExtendedHeaderOffset;
	n64 m_nExtendedHeaderSize;
	n64 m_nLogoRegionOffset;
	n64 m_nLogoRegionSize;
	n64 m_nPlainRegionOffset;
	n64 m_nPlainRegionSize;
	n64 m_nExeFsOffset;
	n64 m_nExeFsSize;
	n64 m_nRomFsOffset;
	n64 m_nRomFsSize;
	bool m_bAlignToBlockSize;
	u8 m_uAesCtr[16];
	const char* m_pXorFileName;
};

#endif	// NCCH_H_
