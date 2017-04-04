#ifndef NCSD_H_
#define NCSD_H_

#include "utility.h"
#include "ncch.h"

#include SDW_MSC_PUSH_PACKED
struct NcsdCommonHeaderStruct
{
	u32 Signature;
	u32 MediaSize;
	u64 MediaId;
	u8 PartitionFsType[8];
	u8 PartitionCryptType[8];
	u32 ParitionOffsetAndSize[16];
	u8 ExtendedHeaderHash[32];
	u32 AdditionalHeaderSize;
	u32 SectorZeroOffset;
	u8 Flags[8];
	u64 PartitionId[8];
	u8 Reserved[48];
} SDW_GNUC_PACKED;

struct SNcsdHeader
{
	u8 RSASignature[256];
	NcsdCommonHeaderStruct Ncsd;
} SDW_GNUC_PACKED;

struct CardInfoHeaderStruct
{
	u64 CardInfo;
	u8 Reserved1[3576];
	u64 MediaId;
	u64 Reserved2;
	u8 InitialData[48];
	u8 Reserved3[192];
	NcchCommonHeaderStruct NcchHeader;
	u8 CardDeviceReserved1[512];
	u8 TitleKey[16];
	u8 CardDeviceReserved2[240];
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

class CNcsd
{
public:
	enum PartitionFsType
	{
		FS_TYPE_DEFAULT
	};
	enum PartitionEncryptoType
	{
		ENCRYPTO_TYPE_DEFAULT
	};
	enum MediaType
	{
		INNER_DEVICE,
		CARD1,
		CARD2,
		EXTENDED_DEVICE
	};
	enum FlagIndex
	{
		MEDIA_CARD_DEVICE = 3,
		MEDIA_PLATFORM_INDEX,
		MEDIA_TYPE_INDEX,
		MEDIA_UNIT_SIZE
	};
	CNcsd();
	~CNcsd();
	void SetFileName(const UString& a_sFileName);
	void SetVerbose(bool a_bVerbose);
	void SetHeaderFileName(const UString& a_sHeaderFileName);
	void SetNcchFileName(const map<int, UString>& a_mNcchFileName);
	void SetNotPad(bool a_bNotPad);
	void SetLastPartitionIndex(int a_nLastPartitionIndex);
	void SetFilePtr(FILE* a_fpNcsd);
	SNcsdHeader& GetNcsdHeader();
	n64* GetOffsetAndSize();
	bool ExtractFile();
	bool CreateFile();
	bool TrimFile();
	bool PadFile();
	void Analyze();
	static bool IsNcsdFile(const UString& a_sFileName);
	static const u32 s_uSignature;
	static const n64 s_nOffsetFirstNcch;
	static const int s_nBlockSize;
private:
	void calculateMediaUnitSize();
	void calculateAlignment();
	void calculateValidSize();
	bool extractFile(const UString& a_sFileName, n64 a_nOffset, n64 a_nSize, const UChar* a_pType, int a_nTypeId, bool bMediaUnitSize);
	bool createHeader();
	bool createNcch(int a_nIndex);
	void clearNcch(int a_nIndex);
	UString m_sFileName;
	bool m_bVerbose;
	UString m_sHeaderFileName;
	map<int, UString> m_mNcchFileName;
	bool m_bNotPad;
	int m_nLastPartitionIndex;
	FILE* m_fpNcsd;
	SNcsdHeader m_NcsdHeader;
	CardInfoHeaderStruct m_CardInfo;
	n64 m_nMediaUnitSize;
	bool m_bAlignToBlockSize;
	n64 m_nValidSize;
	n64 m_nOffsetAndSize[8 * 2];
};

#endif	// NCSD_H_
