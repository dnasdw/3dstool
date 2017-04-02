#include "banner.h"
#include "lz77.h"

const u32 CBanner::s_uSignature = SDW_CONVERT_ENDIAN32('CBMD');
const int CBanner::s_nCbmdSizeAlignment = 0x20;
const char* CBanner::s_pCbmdHeaderFileName = "banner.cbmd";
const char* CBanner::s_pCbmdBodyFileName = "banner%d.bcmdl";
const char* CBanner::s_pBcwavFileName = "banner.bcwav";

CBanner::CBanner()
	: m_pFileName(nullptr)
	, m_bVerbose(false)
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

void CBanner::SetBannerDirName(const string& a_sBannerDirName)
{
	m_sBannerDirName = a_sBannerDirName;
}

bool CBanner::ExtractFile()
{
	bool bResult = true;
	m_fpBanner = Fopen(m_pFileName, "rb");
	if (m_fpBanner == nullptr)
	{
		return false;
	}
	fread(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	if (m_CbmdHeader.CbmdOffset != 0)
	{
		printf("INFO: cbmd offset 0x%" PRIX32 " != 0\n\n", m_CbmdHeader.CbmdOffset);
	}
	if (m_CbmdHeader.CgfxOffset[0] != sizeof(m_CbmdHeader))
	{
		printf("INFO: cgfx 0 offset 0x%" PRIX32 " != 0x%" PRIX32 "\n\n", m_CbmdHeader.CgfxOffset[0], static_cast<u32>(sizeof(m_CbmdHeader)));
	}
	if (!UMakeDir(AToU(m_sBannerDirName).c_str()))
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
	m_fpBanner = Fopen(m_pFileName, "wb");
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
	Fseek(m_fpBanner, 0, SEEK_SET);
	fwrite(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	fclose(m_fpBanner);
	return bResult;
}

bool CBanner::IsBannerFile(const char* a_pFileName)
{
	FILE* fp = Fopen(a_pFileName, "rb");
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
	string sPath = m_sBannerDirName + "/" + s_pCbmdHeaderFileName;
	FILE* fp = Fopen(sPath.c_str(), "wb");
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
	Fseek(m_fpBanner, 0, SEEK_SET);
	u8* pCompressed = new u8[m_CbmdHeader.CwavOffset];
	fread(pCompressed, 1, m_CbmdHeader.CwavOffset, m_fpBanner);
	for (int i = 0; i < SDW_ARRAY_COUNT(m_CbmdHeader.CgfxOffset); i++)
	{
		if (m_CbmdHeader.CgfxOffset[i] != 0)
		{
			string sPath = m_sBannerDirName + "/" + Format(s_pCbmdBodyFileName, i);
			FILE* fp = Fopen(sPath.c_str(), "wb");
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
				u32 uUncompressedSize = 0;
				bResult = CLz77::GetUncompressedSize(pCompressed + m_CbmdHeader.CgfxOffset[i], m_CbmdHeader.CwavOffset - m_CbmdHeader.CgfxOffset[i], uUncompressedSize);
				if (bResult)
				{
					u8* pUncompressed = new u8[uUncompressedSize];
					bResult = CLz77::Uncompress(pCompressed + m_CbmdHeader.CgfxOffset[i], m_CbmdHeader.CwavOffset - m_CbmdHeader.CgfxOffset[i], pUncompressed, uUncompressedSize);
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
				fclose(fp);
			}
			if (!bResult)
			{
				break;
			}
		}
	}
	delete[] pCompressed;
	return bResult;
}

bool CBanner::extractBcwav()
{
	string sPath = m_sBannerDirName + "/" + s_pBcwavFileName;
	FILE* fp = Fopen(sPath.c_str(), "wb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("save: %s\n", sPath.c_str());
	}
	Fseek(m_fpBanner, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(Ftell(m_fpBanner));
	CopyFile(fp, m_fpBanner, m_CbmdHeader.CwavOffset, uFileSize - m_CbmdHeader.CwavOffset);
	fclose(fp);
	return true;
}

bool CBanner::createCbmdHeader()
{
	string sPath = m_sBannerDirName + "/" + s_pCbmdHeaderFileName;
	FILE* fp = Fopen(sPath.c_str(), "rb");
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
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
	Fseek(fp, 0, SEEK_SET);
	fread(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, fp);
	fclose(fp);
	fwrite(&m_CbmdHeader, sizeof(m_CbmdHeader), 1, m_fpBanner);
	return true;
}

bool CBanner::createCbmdBody()
{
	bool bResult = true;
	for (int i = 0; i < SDW_ARRAY_COUNT(m_CbmdHeader.CgfxOffset); i++)
	{
		m_CbmdHeader.CgfxOffset[i] = 0;
		string sPath = m_sBannerDirName + "/" + Format(s_pCbmdBodyFileName, i);
		FILE* fp = fopen(sPath.c_str(), "rb");
		if (fp != nullptr)
		{
			if (m_bVerbose)
			{
				printf("load: %s\n", sPath.c_str());
			}
			Fseek(fp, 0, SEEK_END);
			u32 uFileSize = static_cast<u32>(Ftell(fp));
			Fseek(fp, 0, SEEK_SET);
			u8* pData = new u8[uFileSize];
			fread(pData, 1, uFileSize, fp);
			fclose(fp);
			u32 uCompressedSize = CLz77::GetCompressBoundSize(uFileSize, 1);
			u8* pCompressed = new u8[uCompressedSize];
			bResult = CLz77::CompressLzEx(pData, uFileSize, pCompressed, uCompressedSize, 1);
			if (bResult)
			{
				m_CbmdHeader.CgfxOffset[i] = static_cast<u32>(Ftell(m_fpBanner));
				fwrite(pCompressed, 1, uCompressedSize, m_fpBanner);
			}
			else
			{
				printf("ERROR: compress error\n\n");
			}
			delete[] pCompressed;
			delete[] pData;
			if (!bResult)
			{
				break;
			}
		}
	}
	if (bResult)
	{
		PadFile(m_fpBanner, Align(Ftell(m_fpBanner), s_nCbmdSizeAlignment) - Ftell(m_fpBanner), 0);
		m_CbmdHeader.CwavOffset = static_cast<u32>(Ftell(m_fpBanner));
	}
	return bResult;
}

bool CBanner::createBcwav()
{
	string sPath = m_sBannerDirName + "/" + s_pBcwavFileName;
	FILE* fp = Fopen(sPath.c_str(), "rb");
	if (fp == nullptr)
	{
		return false;
	}
	if (m_bVerbose)
	{
		printf("load: %s\n", sPath.c_str());
	}
	Fseek(fp, 0, SEEK_END);
	u32 uFileSize = static_cast<u32>(Ftell(fp));
	Fseek(fp, 0, SEEK_SET);
	u8* pData = new u8[uFileSize];
	fread(pData, 1, uFileSize, fp);
	fclose(fp);
	fwrite(pData, 1, uFileSize, m_fpBanner);
	delete[] pData;
	return true;
}
