#include "backwardlz77.h"

bool CBackwardLZ77::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = false;
	if (a_uCompressedSize >= sizeof(CompFooter))
	{
		const CompFooter* pCompFooter = reinterpret_cast<const CompFooter*>(a_pCompressed + a_uCompressedSize - sizeof(CompFooter));
		a_uUncompressedSize = a_uCompressedSize + pCompFooter->originalBottom;
		bResult = true;
	}
	return bResult;
}

bool CBackwardLZ77::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = false;
	if (a_uCompressedSize >= sizeof(CompFooter))
	{
		const CompFooter* pCompFooter = reinterpret_cast<const CompFooter*>(a_pCompressed + a_uCompressedSize - sizeof(CompFooter));
		u32 uTop = pCompFooter->bufferTopAndBottom & 0xFFFFFF;
		u32 uBottom = pCompFooter->bufferTopAndBottom >> 24 & 0xFF;
		if (uTop <= a_uCompressedSize && uBottom >= sizeof(CompFooter) && uBottom <= sizeof(CompFooter) + 3 && uTop >= uBottom && a_uCompressedSize + pCompFooter->originalBottom <= a_uUncompressedSize)
		{
			a_uUncompressedSize = a_uCompressedSize + pCompFooter->originalBottom;
			memcpy(a_pUncompressed, a_pCompressed, a_uCompressedSize);
			u8* pDest = a_pUncompressed + a_uUncompressedSize;
			u8* pSrc = a_pUncompressed + a_uCompressedSize - uBottom;
			u8* pEnd = a_pUncompressed + a_uCompressedSize - uTop;
			while (pEnd < pSrc)
			{
				u8 uFlag = *--pSrc;
				for (int i = 0; i < 8; i++)
				{
					if ((uFlag << i & 0x80) == 0)
					{
						*--pDest = *--pSrc;
					}
					else
					{
						int nOffset = *--pSrc;
						nOffset = nOffset << 8 | *--pSrc;
						int nSize = (nOffset >> 12 & 0xF) + 3;
						nOffset = (nOffset & 0xFFF) + 3;
						u8* pData = pDest + nOffset;
						for (int i = 0; i < nSize; i++)
						{
							*--pDest = *--pData;
						}
					}
					if (pSrc <= pEnd)
					{
						break;
					}
				}
			}
			bResult = true;
		}
	}
	return bResult;
}

bool CBackwardLZ77::Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize)
{
	bool bResult = true;
	if (a_uUncompressedSize > sizeof(CompFooter) && a_uCompressedSize >= a_uUncompressedSize)
	{
		u8* pDest = a_pCompressed + a_uUncompressedSize;
		const u8* pSrc = a_pUncompressed + a_uUncompressedSize;
		while (pSrc - a_pUncompressed > 0 && pDest - a_pCompressed > 0)
		{
			u8* pFlag = --pDest;
			*pFlag = 0;
			for (int i = 0; i < 8; i++)
			{
				if (pSrc - a_pUncompressed > 0)
				{
					int nOffsetMax = static_cast<int>(min<ptrdiff_t>(0x1002, a_pUncompressed + a_uUncompressedSize - pSrc));
					int nSizeMax = static_cast<int>(min<ptrdiff_t>(0x12, pSrc - a_pUncompressed));
					nSizeMax = min(nSizeMax, nOffsetMax);
					int nOffset = 0;
					int nSize = 0;
					for (int nCurrentOffset = 3; nCurrentOffset <= nOffsetMax; nCurrentOffset++)
					{
						int nCurrentSizeMax = min(nSizeMax, nCurrentOffset);
						int nCurrentSize = 0;
						for (nCurrentSize = 0; nCurrentSize < nCurrentSizeMax; nCurrentSize++)
						{
							if (*(pSrc - nCurrentSize - 1) != *(pSrc + nCurrentOffset - nCurrentSize - 1))
							{
								break;
							}
						}
						if (nCurrentSize > nSize)
						{
							nSize = nCurrentSize;
							nOffset = nCurrentOffset;
							if (nSize == nSizeMax)
							{
								break;
							}
						}
					}
					if (nSize >= 3)
					{
						if (pDest - a_pCompressed < 2)
						{
							bResult = false;
							break;
						}
						pSrc -= nSize;
						nOffset -= 3;
						nSize -= 3;
						*--pDest = (nSize << 4 & 0xF0) | (nOffset >> 8 & 0xF);
						*--pDest = nOffset & 0xFF;
						*pFlag |= 0x80 >> i;
					}
					else
					{
						if (pDest - a_pCompressed < 1)
						{
							bResult = false;
							break;
						}
						*--pDest = *--pSrc;
					}
				}
				else
				{
					break;
				}
			}
		}
		a_uCompressedSize = static_cast<u32>(a_pCompressed + a_uUncompressedSize - pDest);
	}
	else
	{
		bResult = false;
	}
	if (bResult)
	{
		u32 uOrigSize = a_uUncompressedSize;
		u8* pCompressBuffer = a_pCompressed + a_uUncompressedSize - a_uCompressedSize;
		u32 uCompressBufferSize = a_uCompressedSize;
		u32 uOrigSafe = 0;
		u32 uCompressSafe = 0;
		bool bOver = false;
		while (uOrigSize > 0)
		{
			u8 uFlag = pCompressBuffer[--uCompressBufferSize];
			for (int i = 0; i < 8; i++)
			{
				if ((uFlag << i & 0x80) == 0)
				{
					uCompressBufferSize--;
					uOrigSize--;
				}
				else
				{
					int nOffset = pCompressBuffer[--uCompressBufferSize];
					nOffset = nOffset << 8 | pCompressBuffer[--uCompressBufferSize];
					int nSize = (nOffset >> 12 & 0xF) + 3;
					uOrigSize -= nSize;
					if (uOrigSize < uCompressBufferSize)
					{
						uOrigSafe = uOrigSize;
						uCompressSafe = uCompressBufferSize;
						bOver = true;
						break;
					}
				}
				if (uOrigSize <= 0)
				{
					break;
				}
			}
			if (bOver)
			{
				break;
			}
		}
		u32 uCompressedSize = a_uCompressedSize - uCompressSafe;
		u32 uPadOffset = uOrigSafe + uCompressedSize;
		u32 uCompFooterOffset = static_cast<u32>(FAlign(uPadOffset, 4));
		a_uCompressedSize = uCompFooterOffset + sizeof(CompFooter);
		u32 uTop = a_uCompressedSize - uOrigSafe;
		u32 uBottom = a_uCompressedSize - uPadOffset;
		if (a_uCompressedSize >= a_uUncompressedSize || uTop > 0xFFFFFF)
		{
			bResult = false;
		}
		else
		{
			memcpy(a_pCompressed, a_pUncompressed, uOrigSafe);
			memmove(a_pCompressed + uOrigSafe, pCompressBuffer + uCompressSafe, uCompressedSize);
			memset(a_pCompressed + uPadOffset, 0xFF, uCompFooterOffset - uPadOffset);
			CompFooter* pCompFooter = reinterpret_cast<CompFooter*>(a_pCompressed + uCompFooterOffset);
			pCompFooter->bufferTopAndBottom = uTop | (uBottom << 24);
			pCompFooter->originalBottom = a_uUncompressedSize - a_uCompressedSize;
		}
	}
	return bResult;
}

CBackwardLZ77::CBackwardLZ77()
{
}
