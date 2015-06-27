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
	CBackwardLz77();
};

#endif	// BACKWARDLZ77_H_
