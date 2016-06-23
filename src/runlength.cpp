#include "runlength.h"

bool CRunLength::GetUncompressedSize(const u8* a_pCompressed, u32 a_uCompressedSize, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xFF) == 0x30)
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

u32 CRunLength::GetCompressBoundSize(u32 a_uUncompressedSize, n32 a_nCompressAlign)
{
	return ((a_uUncompressedSize != 0 && a_uUncompressedSize <= 0xFFFFFF ? 4 : 8) + a_uUncompressedSize * 2 + a_nCompressAlign - 1) / a_nCompressAlign * a_nCompressAlign;
}

bool CRunLength::Uncompress(const u8* a_pCompressed, u32 a_uCompressedSize, u8* a_pUncompressed, u32& a_uUncompressedSize)
{
	bool bResult = true;
	if (a_uCompressedSize >= 4 && (a_pCompressed[0] & 0xFF) == 0x30)
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
				while (a_pUncompressed + a_uUncompressedSize - pDest > 0)
				{
					if (a_pCompressed + a_uCompressedSize - pSrc < 1)
					{
						bResult = false;
						break;
					}
					u8 uFlags = *pSrc++;
					int nLength = uFlags & 0x7F;
					if ((uFlags & 0x80) == 0)
					{
						nLength++;
						if (nLength > a_pUncompressed + a_uUncompressedSize - pDest)
						{
							bResult = false;
							break;
						}
						do
						{
							*pDest++ = *pSrc++;
						} while (--nLength > 0);
					}
					else
					{
						nLength += 3;
						if (nLength > a_pUncompressed + a_uUncompressedSize - pDest)
						{
							bResult = false;
							break;
						}
						u8 uSrcTmp = *pSrc++;
						do
						{
							*pDest++ = uSrcTmp;
						} while (--nLength > 0);
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

bool CRunLength::Compress(const u8* a_pUncompressed, u32 a_uUncompressedSize, u8* a_pCompressed, u32& a_uCompressedSize, n32 a_nCompressAlign)
{
	bool bResult = true;
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
			*reinterpret_cast<u32*>(a_pCompressed) = a_uUncompressedSize << 8 | 0x30;
		}
		else
		{
			if (a_uCompressedSize < 8)
			{
				bResult = false;
				break;
			}
			*reinterpret_cast<u32*>(a_pCompressed) = 0x30;
			*reinterpret_cast<u32*>(a_pCompressed + 4) = a_uUncompressedSize;
			nHeaderSize = 8;
		}
		const u8* pSrc = a_pUncompressed;
		u8* pDest = a_pCompressed + nHeaderSize;
		while (a_pUncompressed + a_uUncompressedSize - pSrc > 0)
		{
			if (a_pCompressed + a_uCompressedSize - pDest < 1)
			{
				bResult = false;
				break;
			}
			int nRawDataLength = 0;
			bool bCompFlag = false;
			for (int i = 0; i < 128; i++)
			{
				if (a_pUncompressed + a_uUncompressedSize - (pSrc + nRawDataLength) <= 0)
				{
					nRawDataLength = static_cast<int>(a_pUncompressed + a_uUncompressedSize - pSrc);
					break;
				}
				if (a_pUncompressed + a_uUncompressedSize - (pSrc + nRawDataLength) > 2)
				{
					if (pSrc[i + 1] == pSrc[i] && pSrc[i + 2] == pSrc[i])
					{
						bCompFlag = true;
						break;
					}
				}
				nRawDataLength++;
			}
			if (nRawDataLength != 0)
			{
				if (a_pCompressed + a_uCompressedSize - pDest < nRawDataLength + 1)
				{
					bResult = false;
					break;
				}
				*pDest++ = nRawDataLength - 1;
				for (int i = 0; i < nRawDataLength; i++)
				{
					*pDest++ = *pSrc++;
				}
			}
			if (bCompFlag)
			{
				int nRunLength = 3;
				for (int i = 3; i < 0x7F + 3; i++)
				{
					if (a_pUncompressed + a_uUncompressedSize - (pSrc + nRunLength) <= 0)
					{
						nRunLength = static_cast<int>(a_pUncompressed + a_uUncompressedSize - pSrc);
						break;
					}
					if (pSrc[i] != pSrc[0])
					{
						break;
					}
					nRunLength++;
				}
				if (a_pCompressed + a_uCompressedSize - pDest < 2)
				{
					bResult = false;
					break;
				}
				*pDest++ = 0x80 | (nRunLength - 3);
				*pDest++ = pSrc[0];
				pSrc += nRunLength;
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
	return bResult;
}

CRunLength::CRunLength()
{
}
