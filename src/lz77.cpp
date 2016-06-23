#include "lz77.h"

const int CLz77::s_nCompressWorkSize = (4096 + 256 + 256) * sizeof(n16);

bool CLz77::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xF0) == 0x10 && ((a_pCompressed[0] & 0x0F) == 0 || (a_pCompressed[0] & 0x0F) == 1))
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

u32 CLz77::GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign)
{
	return ((a_uUncompressedSize != 0 && a_uUncompressedSize <= 0xFFFFFF ? 4 : 8) + (a_uUncompressedSize + 7) / 8 * 9 + a_nCompressAlign - 1) / a_nCompressAlign * a_nCompressAlign;
}

bool CLz77::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xF0) == 0x10 && ((a_pCompressed[0] & 0x0F) == 0 || (a_pCompressed[0] & 0x0F) == 1))
	{
		u32 uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed) >> 8 & 0xFFFFFF;
		n32 nHeaderSize = 4;
		if (uUncompressedSize != 0 || a_uCompressedSize >= 8)
		{
			if (uUncompressedSize == 0)
			{
				uUncompressedSize = *reinterpret_cast<const u32*>(a_pCompressed + 4);
				nHeaderSize = 8;
			}
			if (a_uUncompressedSize >= uUncompressedSize)
			{
				a_uUncompressedSize = uUncompressedSize;
				const u8* pSrc = a_pCompressed + nHeaderSize;
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
							u8* pData = pDest - nOffset;
							if (pData < a_pUncompressed)
							{
								bResult = false;
								break;
							}
							for (int j = 0; j < nSize; j++)
							{
								*pDest++ = *pData++;
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

bool CLz77::CompressLz(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, false);
}

bool CLz77::CompressLzEx(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	return compress(a_pUncompressed, a_uUncompressedSize, a_pCompressed, a_uCompressedSize, a_nCompressAlign, true);
}

CLz77::CLz77()
{
}

bool CLz77::compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign, bool a_bExFormat)
{
	bool bResult = true;
	u8* pWork = new u8[s_nCompressWorkSize];
	do
	{
		n32 nHeaderSize = 4;
		if (a_uUncompressedSize != 0 && a_uUncompressedSize <= 0xFFFFFF)
		{
			if (a_uCompressedSize < 4)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = a_uUncompressedSize << 8 | 0x10 | (a_bExFormat ? 1 : 0);
		}
		else
		{
			if (a_uCompressedSize < 8)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = 0x10 | (a_bExFormat ? 1 : 0);
			*reinterpret_cast<u32*>(a_pCompressed + 4) = a_uUncompressedSize;
			nHeaderSize = 8;
		}
		SCompressInfo info;
		initTable(&info, pWork);
		const int nMaxSize = a_bExFormat ? 0xFFFF + 0xFF + 0xF + 3 : 0xF + 3;
		const u8* pSrc = a_pUncompressed;
		u8* pDest = a_pCompressed + nHeaderSize;
		while (a_pUncompressed + a_uUncompressedSize - pSrc > 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			u8* pFlag = pDest++;
			*pFlag = 0;
			for (int i = 0; i < 8; i++)
			{
				int nOffset = 0;
				int nSize = search(&info, pSrc, nOffset, static_cast<int>(min<n64>(nMaxSize, a_pUncompressed + a_uUncompressedSize - pSrc)));
				if (nSize < 3)
				{
					if (a_pCompressed + a_uCompressedSize - pDest < 1)
					{
						bResult = false;
						break;
					}
					slide(&info, pSrc, 1);
					*pDest++ = *pSrc++;
				}
				else
				{
					if (a_pCompressed + a_uCompressedSize - pDest < 2)
					{
						bResult = false;
						break;
					}
					*pFlag |= 0x80 >> i;
					slide(&info, pSrc, nSize);
					pSrc += nSize;
					if (!a_bExFormat)
					{
						nSize -= 3;
					}
					else
					{
						if (nSize >= 0xFF + 0xF + 3)
						{
							nSize -= 0xFF + 0xF + 3;
							*pDest++ = 0x10 | (nSize >> 12 & 0x0F);
							*pDest++ = nSize >> 4 & 0xFF;
						}
						else if (nSize >= 0xF + 2)
						{
							nSize -= 0xF + 2;
							*pDest++ = nSize >> 4 & 0x0F;
						}
						else
						{
							nSize -= 1;
						}
					}
					if (a_pCompressed + a_uCompressedSize - pDest < 2)
					{
						bResult = false;
						break;
					}
					*pDest++ = (nSize << 4 & 0xF0) | ((nOffset - 1) >> 8 & 0x0F);
					*pDest++ = (nOffset - 1) & 0xFF;
				}
				if (a_pUncompressed + a_uUncompressedSize - pSrc <= 0)
				{
					break;
				}
			}
			if (!bResult)
			{
				break;
			}
		}
		if (!bResult)
		{
			break;
		}
		while ((pDest - a_pCompressed) % a_nCompressAlign != 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			*pDest++ = 0;
		}
		if (!bResult)
		{
			break;
		}
		a_uCompressedSize = static_cast<u32>(pDest - a_pCompressed);
	} while (false);
	delete[] pWork;
	return bResult;
}

void CLz77::initTable(SCompressInfo* a_pInfo, void* a_pWork)
{
	a_pInfo->WindowPos = 0;
	a_pInfo->WindowLen = 0;
	a_pInfo->OffsetTable = static_cast<n16*>(a_pWork);
	a_pInfo->ByteTable = static_cast<n16*>(a_pWork) + 4096;
	a_pInfo->EndTable = static_cast<n16*>(a_pWork) + 4096 + 256;
	for (int i = 0; i < 256; i++)
	{
		a_pInfo->ByteTable[i] = -1;
		a_pInfo->EndTable[i] = -1;
	}
}

int CLz77::search(SCompressInfo* a_pInfo, const u8* a_pSrc, int& a_nOffset, int a_nMaxSize)
{
	if (a_nMaxSize < 3)
	{
		return 0;
	}
	const u8* pSearch = nullptr;
	int nSize = 2;
	const u16 uWindowPos = a_pInfo->WindowPos;
	const u16 uWindowLen = a_pInfo->WindowLen;
	n16* pOffsetTable = a_pInfo->OffsetTable;
	for (n16 nOffset = a_pInfo->ByteTable[*a_pSrc]; nOffset != -1; nOffset = pOffsetTable[nOffset])
	{
		if (nOffset < uWindowPos)
		{
			pSearch = a_pSrc - uWindowPos + nOffset;
		}
		else
		{
			pSearch = a_pSrc - uWindowLen - uWindowPos + nOffset;
		}
		if (*(pSearch + 1) != *(a_pSrc + 1) || *(pSearch + 2) != *(a_pSrc + 2))
		{
			continue;
		}
		if (a_pSrc - pSearch < 2)
		{
			break;
		}
		int nCurrentSize = 3;
		while (nCurrentSize < a_nMaxSize && *(pSearch + nCurrentSize) == *(a_pSrc + nCurrentSize))
		{
			nCurrentSize++;
		}
		if (nCurrentSize > nSize)
		{
			nSize = nCurrentSize;
			a_nOffset = static_cast<int>(a_pSrc - pSearch);
			if (nSize == a_nMaxSize)
			{
				break;
			}
		}
	}
	if (nSize < 3)
	{
		return 0;
	}
	return nSize;
}

inline void CLz77::slide(SCompressInfo* a_pInfo, const u8* a_pSrc, int a_nSize)
{
	for (int i = 0; i < a_nSize; i++)
	{
		slideByte(a_pInfo, a_pSrc++);
	}
}

void CLz77::slideByte(SCompressInfo* a_pInfo, const u8* a_pSrc)
{
	u8 uInData = *a_pSrc;
	u16 uInsertOffset = 0;
	const u16 uWindowPos = a_pInfo->WindowPos;
	const u16 uWindowLen = a_pInfo->WindowLen;
	n16* pOffsetTable = a_pInfo->OffsetTable;
	n16* pByteTable = a_pInfo->ByteTable;
	n16* pEndTable = a_pInfo->EndTable;
	if (uWindowLen == 4096)
	{
		u8 uOutData = *(a_pSrc - 4096);
		if ((pByteTable[uOutData] = pOffsetTable[pByteTable[uOutData]]) == -1)
		{
			pEndTable[uOutData] = -1;
		}
		uInsertOffset = uWindowPos;
	}
	else
	{
		uInsertOffset = uWindowLen;
	}
	n16 nOffset = pEndTable[uInData];
	if (nOffset == -1)
	{
		pByteTable[uInData] = uInsertOffset;
	}
	else
	{
		pOffsetTable[nOffset] = uInsertOffset;
	}
	pEndTable[uInData] = uInsertOffset;
	pOffsetTable[uInsertOffset] = -1;
	if (uWindowLen == 4096)
	{
		a_pInfo->WindowPos = (uWindowPos + 1) % 4096;
	}
	else
	{
		a_pInfo->WindowLen++;
	}
}
