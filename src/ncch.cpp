#include "ncch.h"

CNcch::CNcch()
	: m_pFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pRomFsXorFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pAccessControlExtendedFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_bVerbose(false)
	, m_fpNcch(nullptr)
	, m_nMediaUnitSize(1 << 9)
	, m_nExtendedHeaderOffset(0)
	, m_nExtendedHeaderSize(0)
	, m_nAccessControlExtendedOffset(0)
	, m_nAccessControlExtendedSize(0)
	, m_nPlainRegionOffset(0)
	, m_nPlainRegionSize(0)
	, m_nExeFsOffset(0)
	, m_nExeFsSize(0)
	, m_nRomFsOffset(0)
	, m_nRomFsSize(0)
{
	memset(&m_NcchHeader, 0, sizeof(m_NcchHeader));
}

CNcch::~CNcch()
{
}

void CNcch::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CNcch::SetExtendedHeaderXorFileName(const char* a_pExtendedHeaderXorFileName)
{
	m_pExtendedHeaderXorFileName = a_pExtendedHeaderXorFileName;
}

void CNcch::SetExeFsXorFileName(const char* a_pExeFsXorFileName)
{
	m_pExeFsXorFileName = a_pExeFsXorFileName;
}

void CNcch::SetRomFsXorFileName(const char* a_pRomFsXorFileName)
{
	m_pRomFsXorFileName = a_pRomFsXorFileName;
}

void CNcch::SetHeaderFileName(const char* a_pHeaderFileName)
{
	m_pHeaderFileName = a_pHeaderFileName;
}

void CNcch::SetExtendedHeaderFileName(const char* a_pExtendedHeaderFileName)
{
	m_pExtendedHeaderFileName = a_pExtendedHeaderFileName;
}

void CNcch::SetAccessControlExtendedFileName(const char* a_pAccessControlExtendedFileName)
{
	m_pAccessControlExtendedFileName = a_pAccessControlExtendedFileName;
}

void CNcch::SetPlainRegionFileName(const char* a_pPlainRegionFileName)
{
	m_pPlainRegionFileName = a_pPlainRegionFileName;
}

void CNcch::SetExeFsFileName(const char* a_pExeFsFileName)
{
	m_pExeFsFileName = a_pExeFsFileName;
}

void CNcch::SetRomFsFileName(const char* a_pRomFsFileName)
{
	m_pRomFsFileName = a_pRomFsFileName;
}

void CNcch::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CNcch::CryptoFile()
{
	bool bResult = true;
	m_fpNcch = FFoepn(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	fclose(m_fpNcch);
	calculateOffsetSize();
	if (!cryptoFile(m_pExtendedHeaderXorFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, 0, "extendedheader"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pExtendedHeaderXorFileName, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize, m_nExtendedHeaderSize, "accesscontrolextended"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pExeFsXorFileName, m_nExeFsOffset, m_nExeFsSize, 0, "exefs"))
	{
		bResult = false;
	}
	if (!cryptoFile(m_pRomFsXorFileName, m_nRomFsOffset, m_nRomFsSize, 0, "romfs"))
	{
		bResult = false;
	}
	return true;
}

bool CNcch::ExtractFile()
{
	bool bResult = true;
	m_fpNcch = FFoepn(m_pFileName, "rb");
	if (m_fpNcch == nullptr)
	{
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, m_fpNcch);
	calculateOffsetSize();
	if (!extractFile(m_pHeaderFileName, 0, kExtendedHeaderOffset, "ncch header"))
	{
		bResult = false;
	}
	if (!extractFile(m_pExtendedHeaderFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, "extendedheader"))
	{
		bResult = false;
	}
	if (!extractFile(m_pAccessControlExtendedFileName, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize, "accesscontrolextended"))
	{
		bResult = false;
	}
	if (!extractFile(m_pPlainRegionFileName, m_nPlainRegionOffset, m_nPlainRegionSize, "plainregion"))
	{
		bResult = false;
	}
	if (!extractFile(m_pExeFsFileName, m_nExeFsOffset, m_nExeFsSize, "exefs"))
	{
		bResult = false;
	}
	if (!extractFile(m_pRomFsFileName, m_nRomFsOffset, m_nRomFsSize, "romfs"))
	{
		bResult = false;
	}
	fclose(m_fpNcch);
	return bResult;
}

bool CNcch::IsNcchFile(const char* a_pFileName)
{
	FILE* fp = FFoepn(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SNcchHeader ncchHeader;
	fread(&ncchHeader, sizeof(ncchHeader), 1, fp);
	fclose(fp);
	return ncchHeader.Ncch.Signature == s_uSignature;
}

void CNcch::calculateOffsetSize()
{
	m_nMediaUnitSize = 1LL << (m_NcchHeader.Ncch.Flags[6] + 9);
	m_nExtendedHeaderOffset = kExtendedHeaderOffset;
	m_nExtendedHeaderSize = m_NcchHeader.Ncch.ExtendedHeaderSize;
	m_nPlainRegionOffset = m_NcchHeader.Ncch.PlainRegionOffset * m_nMediaUnitSize;
	if (m_nPlainRegionOffset < m_nExtendedHeaderOffset)
	{
		m_nPlainRegionOffset = m_nExtendedHeaderOffset + m_nExtendedHeaderSize;
	}
	m_nPlainRegionSize = m_NcchHeader.Ncch.PlainRegionSize * m_nMediaUnitSize;
	m_nAccessControlExtendedOffset = m_nExtendedHeaderOffset + m_nExtendedHeaderSize;
	m_nAccessControlExtendedSize = m_nPlainRegionOffset - m_nAccessControlExtendedOffset;
	m_nExeFsOffset = m_NcchHeader.Ncch.ExeFsOffset * m_nMediaUnitSize;
	m_nExeFsSize = m_NcchHeader.Ncch.ExeFsSize * m_nMediaUnitSize;
	m_nRomFsOffset = m_NcchHeader.Ncch.RomFsOffset * m_nMediaUnitSize;
	m_nRomFsSize = m_NcchHeader.Ncch.RomFsSize * m_nMediaUnitSize;
}

bool CNcch::cryptoFile(const char* a_pXorFileName, n64 a_nOffset, n64 a_nSize, n64 a_nXorOffset, const char* a_pType)
{
	bool bResult = true;
	if (a_pXorFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			bResult = FCryptoFile(m_pFileName, a_pXorFileName, a_nOffset, a_nSize, false, a_nXorOffset, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists\n", a_pType);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not decrypt or encrypt\n", a_pType);
	}
	return bResult;
}

bool CNcch::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nSize != 0)
		{
			FILE* fp = FFoepn(a_pFileName, "wb");
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					printf("save: %s\n", a_pFileName);
				}
				FCopyFile(fp, m_fpNcch, a_nOffset, a_nSize);
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: %s is not exists, %s will not be create\n", a_pType, a_pFileName);
		}
	}
	else if (a_nSize != 0 && m_bVerbose)
	{
		printf("INFO: %s is not extract\n", a_pType);
	}
	return bResult;
}
