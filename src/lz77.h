#ifndef LZ77_H_
#define LZ77_H_

#include "utility.h"

class CLz77
{
public:
	static bool GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize);
	static u32 GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign);
	static bool Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize);
	static bool CompressLz(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
	static bool CompressLzEx(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign);
private:
	struct SCompressInfo
	{
		u16 WindowPos;
		u16 WindowLen;
		n16* OffsetTable;
		n16* ByteTable;
		n16* EndTable;
	};
	CLz77();
	static bool compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, bool a_bExFormat);
	static void initTable(SCompressInfo* a_pInfo, void* a_pWork);
	static int search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize);
	static inline void slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize);
	static void slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc);
	static const int s_nCompressWorkSize;
};

#endif	// LZ77_H_
