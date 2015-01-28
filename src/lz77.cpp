#include "lz77.h"

bool CLZ77::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xF0) == 0x10 && (a_pCompressed[0] & 0x0F) <= 1)
	{
		a_uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed) >> 8 & 0xFFFFFF;
		if (a_uUncompressedSize != 0 || a_uCompressedSize >= 8)
		{
			if (a_uUncompressedSize == 0)
			{
				a_uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed + 4);
			}
		}
		else
		{
			bResult = false;
		}
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

u32 CLZ77::GetCompressBoundSize(u32 a_uCompressedSize, n32 a_nCompressAlign)
{
	return ((a_uCompressedSize + 7) / 8 * 9 + a_nCompressAlign - 1) / a_nCompressAlign * a_nCompressAlign;
}

bool CLZ77::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xF0) == 0x10 && (a_pCompressed[0] & 0x0F) <= 1)
	{
		u32 uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed) >> 8 & 0xFFFFFF;
		u32 uHeaderSize = 4;
		if (uUncompressedSize != 0 || a_uCompressedSize >= 8)
		{
			if (uUncompressedSize == 0)
			{
				uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed + 4);
				uHeaderSize = 8;
			}
			if (a_uUncompressedSize >= uUncompressedSize)
			{
				a_uUncompressedSize = uUncompressedSize;
				const u8* pSrc = a_pCompressed + uHeaderSize;
				u8* pDest = a_pUncompressed;
				bool bExFormat = (a_pCompressed[0] & 0x0F) != 0;
				while (a_pUncompressed + a_uUncompressedSize - pDest > 0)
				{
					if (a_pCompressed + a_uCompressedSize - pSrc < 1)
					{
						bResult = false;
						break;
					}
					u8 uFlag = *pSrc++;
					for (int i = 0; i < 8; i++)
					{
						if ((uFlag << i & 0x80) == 0)
						{
							if (a_pCompressed + a_uCompressedSize - pSrc < 1)
							{
								bResult = false;
								break;
							}
							*pDest++ = *pSrc++;
						}
						else
						{
							if (a_pCompressed + a_uCompressedSize - pSrc < 1)
							{
								bResult = false;
								break;
							}
							int nSize = *pSrc >> 4 & 0x0F;
							if (!bExFormat)
							{
								nSize += 3;
							}
							else
							{
								if (nSize == 1)
								{
									if (a_pCompressed + a_uCompressedSize - pSrc < 3)
									{
										bResult = false;
										break;
									}
									nSize = (*pSrc++ & 0x0F) << 12;
									nSize |= *pSrc++ << 4;
									nSize |= *pSrc >> 4;
									nSize += 0xFF + 0xF + 3;
								}
								else if (nSize == 0)
								{
									if (a_pCompressed + a_uCompressedSize - pSrc < 2)
									{
										bResult = false;
										break;
									}
									nSize = (*pSrc++ & 0x0F) << 4;
									nSize |= *pSrc >> 4;
									nSize += 0xF + 2;
								}
								else
								{
									nSize += 1;
								}
							}
							if (a_pCompressed + a_uCompressedSize - pSrc < 2)
							{
								bResult = false;
								break;
							}
							int nOffset = (*pSrc++ & 0x0F) << 8;
							nOffset = (nOffset | *pSrc++) + 1;
							if (nSize > a_pUncompressed + a_uUncompressedSize - pDest)
							{
								bResult = false;
								break;
							}
							u8* pDest2 = pDest - nOffset;
							if (pDest2 < a_pUncompressed)
							{
								bResult = false;
								break;
							}
							for (int j = 0; j < nSize; j++)
							{
								*pDest++ = *pDest2++;
							}
						}
						if (a_pUncompressed + a_uUncompressedSize - pDest <= 0)
						{
							break;
						}
					}
					if (!bResult)
					{
						break;
					}
				}
			}
			else
			{
				bResult = false;
			}
		}
		else
		{
			bResult = false;
		}
	}
	else
	{
		bResult = false;
	}
	return bResult;
}

bool CLZ77::CompressLZ(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, false);
}

bool CLZ77::CompressLZEx(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, true);
}

CLZ77::CLZ77()
{
}

bool CLZ77::compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, bool a_bExFormat)
{
	bool bResult = true;
	const u8* pUncompressed = a_pUncompressed;
	u8* pAlignUncompressed = nullptr;
	if (reinterpret_cast<u64>(pUncompressed) % 2 != 0)
	{
		pAlignUncompressed = new u8[a_uUncompressedSize + 1];
		memcpy(pAlignUncompressed + reinterpret_cast<u64>(pAlignUncompressed) % 2, a_pUncompressed, a_uUncompressedSize);
		pUncompressed = pAlignUncompressed + reinterpret_cast<u64>(pAlignUncompressed) % 2;
	}
	u8* pWork = new u8[LZ_COMPRESS_WORK_SIZE];
	do
	{
		u32     LZDstCount;                // Number of bytes of compressed data
		u8      LZCompFlags;               // Flag sequence indicating whether there is a compression
		u8     *LZCompFlagsp;              // Point to memory regions storing LZCompFlags
		u16     lastOffset;                // Offset to matching data (the longest matching data at the time) 
		u32     lastLength;                // Length of matching data (the longest matching data at the time)
		u8      i;
		u32     dstMax;
		LZCompressInfo info;               // Temporary LZ compression information
		const u32 MAX_LENGTH = (a_bExFormat) ? (0xFFFF + 0xFF + 0xF + 3U) : (0xF + 3U);

		if (a_uUncompressedSize <= 0xFFFFFF)
		{
			if (a_uCompressedSize < 4)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = a_uUncompressedSize << 8 | 0x10 | (a_bExFormat ? 1 : 0);  // Data header
			a_pCompressed += 4;
			LZDstCount = 4;
		}
		else
		{
			// Use extended header if the size is larger than 24 bits
			if (a_uCompressedSize < 8)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = 0x10 | (a_bExFormat ? 1 : 0);  // Data header
			a_pCompressed += 4;
			*reinterpret_cast<u32*>(a_pCompressed) = a_uUncompressedSize; // Size extended header
			a_pCompressed += 4;
			LZDstCount = 8;
		}
		dstMax = a_uCompressedSize;
		LZInitTable(&info, pWork);

		while (a_uUncompressedSize > 0)
		{
			LZCompFlags = 0;
			LZCompFlagsp = a_pCompressed++;         // Destination for storing flag sequence
			LZDstCount++;

			// Because flag sequence is stored as 8-bit data, loop eight times
			for (i = 0; i < 8; i++)
			{
				LZCompFlags <<= 1;         // No meaning for the first time (i=0)
				if (a_uUncompressedSize <= 0)
				{
					// When reached the end, quit after shifting flag to the end
					continue;
				}

				if ((lastLength = SearchLZ(&info, pUncompressed, a_uUncompressedSize, &lastOffset, MAX_LENGTH)) != 0)
				{
					u32 length;
					// Enable flag if compression is possible
					LZCompFlags |= 0x1;

					if (LZDstCount + 2 > dstMax)   // Quit on error if size becomes larger than source
					{
						bResult = false;
						break;
					}

					if (a_bExFormat)
					{
						if (lastLength >= 0xFF + 0xF + 3)
						{
							length = static_cast<u32>(lastLength - 0xFF - 0xF - 3);
							*a_pCompressed++ = static_cast<u8>(0x10 | (length >> 12));
							*a_pCompressed++ = static_cast<u8>(length >> 4);
							LZDstCount += 2;
						}
						else if (lastLength >= 0xF + 2)
						{
							length = static_cast<u32>(lastLength - 0xF - 2);
							*a_pCompressed++ = static_cast<u8>(length >> 4);
							LZDstCount += 1;
						}
						else
						{
							length = static_cast<u32>(lastLength - 1);
						}
					}
					else
					{
						length = static_cast<u32>(lastLength - 3);
					}

					if (LZDstCount + 2 > dstMax)   // Quit on error if size becomes larger than source
					{
						bResult = false;
						break;
					}

					// Divide offset into upper 4 bits and lower 8 bits and store
					*a_pCompressed++ = static_cast<u8>(length << 4 | (lastOffset - 1) >> 8);
					*a_pCompressed++ = static_cast<u8>((lastOffset - 1) & 0xff);
					LZDstCount += 2;
					LZSlide(&info, pUncompressed, lastLength);
					pUncompressed += lastLength;
					a_uUncompressedSize -= lastLength;
				}
				else
				{
					// No compression
					if (LZDstCount + 1 > dstMax)       // Quit on error if size becomes larger than source
					{
						bResult = false;
						break;
					}
					LZSlide(&info, pUncompressed, 1);
					*a_pCompressed++ = *pUncompressed++;
					a_uUncompressedSize--;
					LZDstCount++;
				}
			}                              // Completed eight loops
			if (!bResult)
			{
				break;
			}
			*LZCompFlagsp = LZCompFlags;   // Store flag series
		}
		if (!bResult)
		{
			break;
		}
		while (LZDstCount % a_nCompressAlign != 0)
		{
			if (LZDstCount + 1 > dstMax)
			{
				bResult = false;
				break;
			}
			*a_pCompressed++ = 0;
			LZDstCount++;
		}
		if (!bResult)
		{
			break;
		}
		a_uCompressedSize = LZDstCount;
	} while (false);
	delete[] pWork;
	if (pAlignUncompressed != nullptr)
	{
		delete[] pAlignUncompressed;
	}
	return bResult;
}

//--------------------------------------------------------
// With LZ77 Compression, searches for the longest matching string from the slide window.
//  Arguments:    startp                 Pointer to starting position of data
//                nextp                  Pointer to data where search will start
//                remainSize             Size of remaining data
//                offset                 Pointer to region storing matched offset
//  Return:    TRUE if matching string is found
//                FALSE if not found.
//--------------------------------------------------------
u32 CLZ77::SearchLZ(LZCompressInfo * info, const u8 *nextp, u32 remainSize, u16 *offset, u32 maxLength)
{
	const u8 *searchp;
	const u8 *headp, *searchHeadp;
	u16     maxOffset = 0;
	u32     currLength = 2;
	u32     tmpLength;
	s32     w_offset;
	s16    *const LZOffsetTable = info->LZOffsetTable;
	const u16 windowPos = info->windowPos;
	const u16 windowLen = info->windowLen;

	if (remainSize < 3)
	{
		return 0;
	}

	w_offset = info->LZByteTable[*nextp];

	while (w_offset != -1)
	{
		if (w_offset < windowPos)
		{
			searchp = nextp - windowPos + w_offset;
		}
		else
		{
			searchp = nextp - windowLen - windowPos + w_offset;
		}

		/* This isn't needed, but it seems to make it a little faster */
		if (*(searchp + 1) != *(nextp + 1) || *(searchp + 2) != *(nextp + 2))
		{
			w_offset = LZOffsetTable[w_offset];
			continue;
		}

		if (nextp - searchp < 2)
		{
			// Since VRAM is 2-byte access (since there are times when data is read from VRAM), the search target data must be set to data that is two bytes before that.
			// 
			// 
			// Because the offset is stored in 12 bits, the value is 4096 or less
			break;
		}
		tmpLength = 3;
		searchHeadp = searchp + 3;
		headp = nextp + 3;

		// Increments the compression size until the data ends or different data is encountered
		while ((static_cast<u32>(headp - nextp) < remainSize) && (*headp == *searchHeadp))
		{
			headp++;
			searchHeadp++;
			tmpLength++;

			// Because the data length is stored in 4 bits, the value is 18 or less (3 is added)
			if (tmpLength == maxLength)
			{
				break;
			}
		}

		if (tmpLength > currLength)
		{
			// Update the maximum-length offset
			currLength = tmpLength;
			maxOffset = static_cast<u16>(nextp - searchp);
			if (currLength == maxLength || currLength == remainSize)
			{
				// This is the longest matching length, so end search
				break;
			}
		}
		w_offset = LZOffsetTable[w_offset];
	}

	if (currLength < 3)
	{
		return 0;
	}
	*offset = maxOffset;
	return currLength;
}

//--------------------------------------------------------
// Initialize the dictionary index
//--------------------------------------------------------
void CLZ77::LZInitTable(LZCompressInfo * info, void *work)
{
	u16     i;

	info->LZOffsetTable = static_cast<s16*>(work);
	info->LZByteTable = static_cast<s16*>(work) + 4096;
	info->LZEndTable = static_cast<s16*>(work) + 4096 + 256;

	for (i = 0; i < 256; i++)
	{
		info->LZByteTable[i] = -1;
		info->LZEndTable[i] = -1;
	}
	info->windowPos = 0;
	info->windowLen = 0;
}

//--------------------------------------------------------
// Slide the dictionary 1 byte
//--------------------------------------------------------
void CLZ77::SlideByte(LZCompressInfo * info, const u8 *srcp)
{
	s16     offset;
	u8      in_data = *srcp;
	u16     insert_offset;

	s16    *const LZByteTable = info->LZByteTable;
	s16    *const LZOffsetTable = info->LZOffsetTable;
	s16    *const LZEndTable = info->LZEndTable;
	const u16 windowPos = info->windowPos;
	const u16 windowLen = info->windowLen;

	if (windowLen == 4096)
	{
		u8      out_data = *(srcp - 4096);
		if ((LZByteTable[out_data] = LZOffsetTable[LZByteTable[out_data]]) == -1)
		{
			LZEndTable[out_data] = -1;
		}
		insert_offset = windowPos;
	}
	else
	{
		insert_offset = windowLen;
	}

	offset = LZEndTable[in_data];
	if (offset == -1)
	{
		LZByteTable[in_data] = static_cast<s16>(insert_offset);
	}
	else
	{
		LZOffsetTable[offset] = static_cast<s16>(insert_offset);
	}
	LZEndTable[in_data] = static_cast<s16>(insert_offset);
	LZOffsetTable[insert_offset] = -1;

	if (windowLen == 4096)
	{
		info->windowPos = static_cast<s16>((windowPos + 1) % 0x1000);
	}
	else
	{
		info->windowLen++;
	}
}

//--------------------------------------------------------
// Slide the dictionary n bytes
//--------------------------------------------------------
inline void CLZ77::LZSlide(LZCompressInfo * info, const u8 *srcp, u32 n)
{
	u32     i;

	for (i = 0; i < n; i++)
	{
		SlideByte(info, srcp++);
	}
}
