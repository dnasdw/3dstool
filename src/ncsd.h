#ifndef NCSD_H_
#define NCSD_H_

#include "utility.h"

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
	void SetFileName(const char* a_pFileName);
	void SetHeaderFileName(const char* a_pHeaderFileName);
	void SetNcchFileName(const char* a_pNcchFileName[]);
	void SetVerbose(bool a_bVerbose);
	bool ExtractFile();
	static bool IsNcsdFile(const char* a_pFileName);
	static const u32 s_uSignature = CONVERT_ENDIAN('NCSD');
	static const int kOffsetFirstNcch = 0x4000;
private:
	bool extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType, int a_nTypeId, bool bMediaUnitSize);
	const char* m_pFileName;
	const char* m_pHeaderFileName;
	const char* m_pNcchFileName[8];
	bool m_bVerbose;
	FILE* m_fpNcsd;
	SNcsdHeader m_NcsdHeader;
	n64 m_nMediaUnitSize;
};

#endif	// NCSD_H_
