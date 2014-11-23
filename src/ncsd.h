#ifndef NCSD_H_
#define NCSD_H_

#include "utility.h"
#include "ncch.h"

#include MSC_PUSH_PACKED
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
} GNUC_PACKED;

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
} GNUC_PACKED;

struct SNcsdHeader
{
	u8 RSASignature[256];
	NcsdCommonHeaderStruct Ncsd;
} GNUC_PACKED;
#include MSC_POP_PACKED

class CNcsd
{
public:
	CNcsd();
	~CNcsd();
	void SetLastPartitionIndex(int a_nLastPartitionIndex);
	void SetFileName(const char* a_pFileName);
	void SetHeaderFileName(const char* a_pHeaderFileName);
	void SetNcchFileName(const char* a_pNcchFileName[]);
	void SetNotPad(bool a_bNotPad);
	void SetVerbose(bool a_bVerbose);
	bool ExtractFile();
	bool CreateFile();
	bool RipFile();
	bool PadFile();
	static bool IsNcsdFile(const char* a_pFileName);
	static const u32 s_uSignature;
	static const n64 s_nOffsetFirstNcch;
	static const int s_nBlockSize;
private:
	void calculateMediaUnitSize();
	void calculateValidSize();
	bool extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType, int a_nTypeId, bool bMediaUnitSize);
	bool createHeader();
	bool createNcch(int a_nIndex);
	void clearNcch(int a_nIndex);
	int m_nLastPartitionIndex;
	const char* m_pFileName;
	const char* m_pHeaderFileName;
	const char* m_pNcchFileName[8];
	bool m_bNotPad;
	bool m_bVerbose;
	FILE* m_fpNcsd;
	SNcsdHeader m_NcsdHeader;
	CardInfoHeaderStruct m_CardInfo;
	n64 m_nMediaUnitSize;
	n64 m_nValidSize;
};

#endif	// NCSD_H_
