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
	FILE* fp = fopen(m_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", m_pFileName);
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	fclose(fp);
	calculateOffsetSize();
	if (m_pExtendedHeaderXorFileName != nullptr)
	{
		if (m_nExtendedHeaderSize != 0)
		{
			FCryptoFile(m_pFileName, m_pExtendedHeaderXorFileName, m_nExtendedHeaderOffset, m_nExtendedHeaderSize, false, 0, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: extendedheader is not exists\n");
		}
		if (m_nAccessControlExtendedSize != 0)
		{
			FCryptoFile(m_pFileName, m_pExtendedHeaderXorFileName, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize, false, m_nExtendedHeaderSize, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: accesscontrolextended is not exists\n");
		}
	}
	else if (m_nExtendedHeaderSize != 0 && m_bVerbose)
	{
		printf("INFO: extendedheader is not decrypt or encrypt\n");
	}
	else if (m_nAccessControlExtendedSize != 0 && m_bVerbose)
	{
		printf("INFO: accesscontrolextended is not decrypt or encrypt\n");
	}
	if (m_pExeFsXorFileName != nullptr)
	{
		if (m_nExeFsSize != 0)
		{
			FCryptoFile(m_pFileName, m_pExeFsXorFileName, m_nExeFsOffset, m_nExeFsSize, false, 0, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: exefs is not exists\n");
		}
	}
	else if (m_nExeFsSize != 0 && m_bVerbose)
	{
		printf("INFO: exefs is not decrypt or encrypt\n");
	}
	if (m_pRomFsXorFileName != nullptr)
	{
		if (m_nRomFsSize != 0)
		{
			FCryptoFile(m_pFileName, m_pRomFsXorFileName, m_nRomFsOffset, m_nRomFsSize, false, 0, m_bVerbose);
		}
		else if (m_bVerbose)
		{
			printf("INFO: romfs is not exists\n");
		}
	}
	else if (m_nRomFsSize != 0 && m_bVerbose)
	{
		printf("INFO: romfs is not decrypt or encrypt\n");
	}
	return true;
}

bool CNcch::ExtractFile()
{
	FILE* fp = fopen(m_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", m_pFileName);
		return false;
	}
	fread(&m_NcchHeader, sizeof(m_NcchHeader), 1, fp);
	calculateOffsetSize();
	if (m_pHeaderFileName != nullptr)
	{
		FILE* fpHeader = fopen(m_pHeaderFileName, "wb");
		if (fpHeader == nullptr)
		{
			printf("ERROR: create file %s failed\n\n", m_pHeaderFileName);
		}
		else
		{
			if (m_bVerbose)
			{
				printf("create: %s\n", m_pHeaderFileName);
			}
			FCopyFile(fpHeader, fp, 0, kExtendedHeaderOffset);
			fclose(fpHeader);
		}
	}
	else if (m_bVerbose)
	{
		printf("INFO: ncch header is not extract\n");
	}
	if (m_pExtendedHeaderFileName != nullptr)
	{
		if (m_nExtendedHeaderSize != 0)
		{
			FILE* fpExtendedHeader = fopen(m_pExtendedHeaderFileName, "wb");
			if (fpExtendedHeader == nullptr)
			{
				printf("ERROR: create file %s failed\n\n", m_pExtendedHeaderFileName);
			}
			else
			{
				if (m_bVerbose)
				{
					printf("create: %s\n", m_pExtendedHeaderFileName);
				}
				FCopyFile(fpExtendedHeader, fp, m_nExtendedHeaderOffset, m_nExtendedHeaderSize);
				fclose(fpExtendedHeader);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: extendedheader is not exists, %s will not be create\n", m_pExtendedHeaderFileName);
		}
	}
	else if (m_nExtendedHeaderSize != 0 && m_bVerbose)
	{
		printf("INFO: extendedheader is not extract\n");
	}
	if (m_pAccessControlExtendedFileName != nullptr)
	{
		if (m_nAccessControlExtendedSize != 0)
		{
			FILE* fpAccessControlExtended = fopen(m_pAccessControlExtendedFileName, "wb");
			if (fpAccessControlExtended == nullptr)
			{
				printf("ERROR: create file %s failed\n\n", m_pAccessControlExtendedFileName);
			}
			else
			{
				if (m_bVerbose)
				{
					printf("create: %s\n", m_pAccessControlExtendedFileName);
				}
				FCopyFile(fpAccessControlExtended, fp, m_nAccessControlExtendedOffset, m_nAccessControlExtendedSize);
				fclose(fpAccessControlExtended);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: accesscontrolextended is not exists, %s will not be create\n", m_pAccessControlExtendedFileName);
		}
	}
	else if (m_nAccessControlExtendedSize != 0 && m_bVerbose)
	{
		printf("INFO: accesscontrolextended is not extract\n");
	}
	if (m_pPlainRegionFileName != nullptr)
	{
		if (m_nPlainRegionSize != 0)
		{
			FILE* fpPlainRegion = fopen(m_pPlainRegionFileName, "wb");
			if (fpPlainRegion == nullptr)
			{
				printf("ERROR: create file %s failed\n\n", m_pPlainRegionFileName);
			}
			else
			{
				if (m_bVerbose)
				{
					printf("create: %s\n", m_pPlainRegionFileName);
				}
				FCopyFile(fpPlainRegion, fp, m_nPlainRegionOffset, m_nPlainRegionSize);
				fclose(fpPlainRegion);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: plainregion is not exists, %s will not be create\n", m_pPlainRegionFileName);
		}
	}
	else if (m_nPlainRegionSize != 0 && m_bVerbose)
	{
		printf("INFO: plainregion is not extract\n");
	}
	if (m_pExeFsFileName != nullptr)
	{
		if (m_nExeFsSize != 0)
		{
			FILE* fpExeFs = fopen(m_pExeFsFileName, "wb");
			if (fpExeFs == nullptr)
			{
				printf("ERROR: create file %s failed\n\n", m_pExeFsFileName);
			}
			else
			{
				if (m_bVerbose)
				{
					printf("create: %s\n", m_pExeFsFileName);
				}
				FCopyFile(fpExeFs, fp, m_nExeFsOffset, m_nExeFsSize);
				fclose(fpExeFs);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: exefs is not exists, %s will not be create\n", m_pExeFsFileName);
		}
	}
	else if (m_nExeFsSize != 0 && m_bVerbose)
	{
		printf("INFO: exefs is not extract\n");
	}
	if (m_pRomFsFileName != nullptr)
	{
		if (m_nRomFsSize != 0)
		{
			FILE* fpRomFs = fopen(m_pRomFsFileName, "wb");
			if (fpRomFs == nullptr)
			{
				printf("ERROR: create file %s failed\n\n", m_pRomFsFileName);
			}
			else
			{
				if (m_bVerbose)
				{
					printf("create: %s\n", m_pRomFsFileName);
				}
				FCopyFile(fpRomFs, fp, m_nRomFsOffset, m_nRomFsSize);
				fclose(fpRomFs);
			}
		}
		else if (m_bVerbose)
		{
			printf("INFO: romfs is not exists, %s will not be create\n", m_pRomFsFileName);
		}
	}
	else if (m_nRomFsSize != 0 && m_bVerbose)
	{
		printf("INFO: romfs is not extract\n");
	}
	fclose(fp);
	return true;
}

bool CNcch::IsNcchFile(const char* a_pFileName)
{
	FILE* fp = fopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
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
