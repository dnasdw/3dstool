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
	u8 Reserved1[47];
	u8 ProductCode[16];
	u8 ExtendedHeaderHash[32];
	u32 ExtendedHeaderSize;
	u8 Reserved2[4];
	u8 Flags[8];
	u32 PlainRegionOffset;
	u32 PlainRegionSize;
	u8 Reserved3[8];
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
	CNcch();
	~CNcch();
	void SetFileName(const char* a_pFileName);
	void SetExtendedHeaderXorFileName(const char* a_pExtendedHeaderXorFileName);
	void SetExeFsXorFileName(const char* a_pExeFsXorFileName);
	void SetRomFsXorFileName(const char* a_pRomFsXorFileName);
	void SetHeaderFileName(const char* a_pHeaderFileName);
	void SetExtendedHeaderFileName(const char* a_pExtendedHeaderFileName);
	void SetAccessControlExtendedFileName(const char* a_pAccessControlExtendedFileName);
	void SetPlainRegionFileName(const char* a_pPlainRegionFileName);
	void SetExeFsFileName(const char* a_pExeFsFileName);
	void SetRomFsFileName(const char* a_pRomFsFileName);
	void SetNotUpdateExtendedHeaderHash(bool a_bNotUpdateExtendedHeaderHash);
	void SetNotUpdateExeFsHash(bool a_bNotUpdateExeFsHash);
	void SetNotUpdateRomFsHash(bool a_bNotUpdateRomFsHash);
	void SetVerbose(bool a_bVerbose);
	bool CryptoFile();
	bool ExtractFile();
	bool CreateFile();
	static bool IsNcchFile(const char* a_pFileName);
	static const u32 s_uSignature;
	static const int s_nBlockSize;
private:
	void calculateMediaUnitSize();
	void calculateOffsetSize();
	bool cryptoFile(const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType);
	bool extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType);
	bool createHeader();
	bool createExtendedHeader();
	bool createAccessControlExtended();
	bool createPlainRegion();
	bool createExeFs();
	bool createRomFs();
	void clearExtendedHeader();
	void clearPlainRegion();
	void clearExeFs();
	void clearRomFs();
	void alignFileSize(n64 a_nAlignment);
	const char* m_pFileName;
	const char* m_pExtendedHeaderXorFileName;
	const char* m_pExeFsXorFileName;
	const char* m_pRomFsXorFileName;
	const char* m_pHeaderFileName;
	const char* m_pExtendedHeaderFileName;
	const char* m_pAccessControlExtendedFileName;
	const char* m_pPlainRegionFileName;
	const char* m_pExeFsFileName;
	const char* m_pRomFsFileName;
	bool m_bNotUpdateExtendedHeaderHash;
	bool m_bNotUpdateExeFsHash;
	bool m_bNotUpdateRomFsHash;
	bool m_bVerbose;
	FILE* m_fpNcch;
	SNcchHeader m_NcchHeader;
	n64 m_nMediaUnitSize;
	n64 m_nExtendedHeaderOffset;
	n64 m_nExtendedHeaderSize;
	n64 m_nAccessControlExtendedOffset;
	n64 m_nAccessControlExtendedSize;
	n64 m_nPlainRegionOffset;
	n64 m_nPlainRegionSize;
	n64 m_nExeFsOffset;
	n64 m_nExeFsSize;
	n64 m_nRomFsOffset;
	n64 m_nRomFsSize;
};

#endif	// NCCH_H_
