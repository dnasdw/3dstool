#ifndef BACKWARDLZ77_H_
#define BACKWARDLZ77_H_

#include "utility.h"

#include MSC_PUSH_PACKED
struct CompFooter
{
	u32 bufferTopAndBottom;
	u32 originalBottom;
} GNUC_PACKED;
#include MSC_POP_PACKED

class CBackwardLz77
{
public:
	static bool GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize);
private:
	struct SCompressInfo
	{
		u16 WindowPos;
		u16 WindowLen;
		n16* OffsetTable;
		n16* ReversedOffsetTable;
		n16* ByteTable;
		n16* EndTable;
	};
	static const int s_nCompressWorkSize = (4098 + 4098 + 256 + 256) * sizeof(n16);
	CBackwardLz77();
	static void initTable(SCompressInfo* a_pInfo, void* a_pWork);
	static int search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize);
	static inline void slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize);
	static void slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc);
};

#endif	// BACKWARDLZ77_H_
