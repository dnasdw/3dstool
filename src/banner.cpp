#include "banner.h"
#include "lz77.h"

const u32 CBanner::s_uSignature = CONVERT_ENDIAN('CBMD');
const int CBanner::s_nCbmdSizeAlignment = 0x20;
const char* CBanner::s_pCbmdHeaderFileName = "banner.cbmd";
const char* CBanner::s_pCbmdBodyFileName = "banner.bcmdl";
const char* CBanner::s_pBcwavFileName = "banner.bcwav";

CBanner::CBanner()
	: m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_nCompressAlign(1)
	, m_pBannerDirName(nullptr)
	, m_bUncompress(false)
	, m_bCompress(false)
	, m_fpBanner(nullptr)
{
	memset(&m_CbmdHeader, 0, sizeof(m_CbmdHeader));
}

CBanner::~CBanner()
{
}

void CBanner::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CBanner::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CBanner::SetCompressAlign(n32 a_nCompressAlign)
{
	m_nCompressAlign = a_nCompressAlign;
}

void CBanner::SetBannerDirName(const char* a_pBannerDirName)
{
	m_pBannerDirName = a_pBannerDirName;
}

void CBanner::SetUncompress(bool a_bUncompress)
{
	m_bUncompress = a_bUncompress;
}

void CBanner::SetCompress(bool a_bCompress)
{
	m_bCompress = a_bCompress;
}

bool CBanner::ExtractFile()
{
	bool bResult = true;
	m_fpBanner = FFopen(m_pFileName, "rb");
	if (m_fpBanner == nullptr)
	{
		return false;
	}
	fread(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	if (!FMakeDir(FSAToUnicode(m_pBannerDirName).c_str()))
	{
		fclose(m_fpBanner);
		return false;
	}
	if (!extractCbmdHeader())
	{
		bResult = false;
	}
	if (!extractCbmdBody())
	{
		bResult = false;
	}
	if (!extractBcwav())
	{
		bResult = false;
	}
	fclose(m_fpBanner);
	return bResult;
}

bool CBanner::CreateFile()
{
	bool bResult = true;
	m_fpBanner = FFopen(m_pFileName, "wb");
	if (m_fpBanner == nullptr)
	{
		return false;
	}
	if (!createCbmdHeader())
	{
		fclose(m_fpBanner);
		return false;
	}
	if (!createCbmdBody())
	{
		bResult = false;
	}
	if (!createBcwav())
	{
		bResult = false;
	}
	FFseek(m_fpBanner, 0, SEEK_SET);
	fwrite(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	fclose(m_fpBanner);
	return bResult;
}

bool CBanner::IsBannerFile(const char* a_pFileName)
{
	FILE* fp = FFopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SCbmdHeader cbmdHeader;
	fread(&cbmdHeader, sizeof(cbmdHeader), 1, fp);
	fclose(fp);
	return cbmdHeader.Signature == s_uSignature;
}

bool CBanner::extractCbmdHeader()
{
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pCbmdHeaderFileName;
	FILE* fp = FFopen(sPath.c_str(), "wb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("save: %s\n", sPath.c_str());
	}
	fwrite(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, fp);
	fclose(fp);
	return true;
}

bool CBanner::extractCbmdBody()
{
	bool bResult = true;
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pCbmdBodyFileName;
	FILE* fp = FFopen(sPath.c_str(), "wb");
	if (fp == nullptr)
	{
		bResult = false;
	}
	else
	{
		if (m_bVerbose)
		{
			printf("save: %s\n", sPath.c_str());
		}
		if (m_bUncompress)
		{
			u32 uCompressedSize = m_CbmdHeader.FileSize - m_CbmdHeader.Offset;
			FFseek(m_fpBanner, m_CbmdHeader.Offset, SEEK_SET);
			u8* pCompressed = new u8[uCompressedSize];
			fread(pCompressed, 1, uCompressedSize, m_fpBanner);
			u32 uUncompressedSize = 0;
			bResult = CLz77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			if (bResult)
			{
				u8* pUncompressed = new u8[uUncompressedSize];
				bResult = CLz77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				if (bResult)
				{
					fwrite(pUncompressed, 1, uUncompressedSize, fp);
				}
				else
				{
					printf("ERROR: uncompress error\n\n");
				}
				delete[] pUncompressed;
			}
			else
			{
				printf("ERROR: get uncompressed size error\n\n");
			}
			delete[] pCompressed;
		}
		if (!m_bUncompress || !bResult)
		{
			FCopyFile(fp, m_fpBanner, m_CbmdHeader.Offset, m_CbmdHeader.FileSize - m_CbmdHeader.Offset);
		}
		fclose(fp);
	}
	return bResult;
}

bool CBanner::extractBcwav()
{
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pBcwavFileName;
	FILE* fp = FFopen(sPath.c_str(), "wb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("save: %s\n", sPath.c_str());
	}
	FFseek(m_fpBanner, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(FFtell(m_fpBanner));
	FCopyFile(fp, m_fpBanner, m_CbmdHeader.FileSize, uFileSize - m_CbmdHeader.FileSize);
	fclose(fp);
	return true;
}

bool CBanner::createCbmdHeader()
{
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pCbmdHeaderFileName;
	FILE* fp = FFopen(sPath.c_str(), "rb");
	if (fp == nullptr)
	{
		return false;
	}
	FFseek(fp, 0, SEEK_END);
	n64 nFileSize = FFtell(fp);
	if (nFileSize < sizeof(m_CbmdHeader))
	{
		fclose(fp);
		printf("ERROR: cbmd header is too short\n\n");
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", sPath.c_str());
	}
	FFseek(fp, 0, SEEK_SET);
	fread(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, fp);
	fclose(fp);
	fwrite(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	return true;
}

bool CBanner::createCbmdBody()
{
	bool bResult = true;
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pCbmdBodyFileName;
	FILE* fp = FFopen(sPath.c_str(), "rb");
	if (fp == nullptr)
	{
		bResult = false;
	}
	else
	{
		if (m_bVerbose)
		{
			printf("load: %s\n", sPath.c_str());
		}
		FFseek(fp, 0, SEEK_END);
		u32 uFileSize = static_cast<u32>(FFtell(fp));
		FFseek(fp, 0, SEEK_SET);
		u8* pData = new u8[uFileSize];
		fread(pData, 1, uFileSize, fp);
		fclose(fp);
		bool bCompressResult = false;
		if (m_bCompress)
		{
			u32 uCompressedSize = CLz77::GetCompressBoundSize(uFileSize, m_nCompressAlign);
			u8* pCompressed = new u8[uCompressedSize];
			bCompressResult = CLz77::CompressLzEx(pData, uFileSize, pCompressed, uCompressedSize, m_nCompressAlign);
			if (bCompressResult)
			{
				fwrite(pCompressed, 1, uCompressedSize, m_fpBanner);
			}
			delete[] pCompressed;
		}
		if (!m_bCompress || !bCompressResult)
		{
			fwrite(pData, 1, uFileSize, m_fpBanner);
		}
		delete[] pData;
	}
	FPadFile(m_fpBanner, FAlign(FFtell(m_fpBanner), s_nCbmdSizeAlignment) - FFtell(m_fpBanner), 0);
	m_CbmdHeader.FileSize = static_cast<u32>(FFtell(m_fpBanner));
	return bResult;
}

bool CBanner::createBcwav()
{
	string sPath = m_pBannerDirName;
	sPath += "/";
	sPath += s_pBcwavFileName;
	FILE* fp = FFopen(sPath.c_str(), "rb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", sPath.c_str());
	}
	FFseek(fp, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(FFtell(fp));
	FFseek(fp, 0, SEEK_SET);
	u8* pData = new u8[uFileSize];
	fread(pData, 1, uFileSize, fp);
	fclose(fp);
	fwrite(pData, 1, uFileSize, m_fpBanner);
	delete[] pData;
	return true;
}
