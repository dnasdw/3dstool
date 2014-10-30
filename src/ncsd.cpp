#include "ncsd.h"

CNcsd::CNcsd()
	: m_pFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_bVerbose(false)
	, m_fpNcsd(nullptr)
	, m_nMediaUnitSize(1 << 9)
{
	memset(m_pNcchFileName, 0, sizeof(m_pNcchFileName));
	memset(&m_NcsdHeader, 0, sizeof(m_NcsdHeader));
}

CNcsd::~CNcsd()
{
}

void CNcsd::SetFileName(const char* a_pFileName)
{
	m_pFileName = a_pFileName;
}

void CNcsd::SetHeaderFileName(const char* a_pHeaderFileName)
{
	m_pHeaderFileName = a_pHeaderFileName;
}

void CNcsd::SetNcchFileName(const char* a_pNcchFileName[])
{
	memcpy(m_pNcchFileName, a_pNcchFileName, sizeof(m_pNcchFileName));
}

void CNcsd::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

bool CNcsd::ExtractFile()
{
	bool bResult = true;
	m_fpNcsd = FFoepn(m_pFileName, "rb");
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	m_nMediaUnitSize = 1LL << (m_NcsdHeader.Ncsd.Flags[6] + 9);
	if (!extractFile(m_pHeaderFileName, 0, kOffsetFirstNcch, "ncsd header", -1, false))
	{
		bResult = false;
	}
	for (int i = 0; i < 8; i++)
	{
		if (!extractFile(m_pNcchFileName[i], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1], "partition", i, true))
		{
			bResult = false;
		}
	}
	fclose(m_fpNcsd);
	return bResult;
}

bool CNcsd::IsNcsdFile(const char* a_pFileName)
{
	FILE* fp = FFoepn(a_pFileName, "rb");
	if (fp == nullptr)
	{
		return false;
	}
	SNcsdHeader ncsdHeader;
	fread(&ncsdHeader, sizeof(ncsdHeader), 1, fp);
	fclose(fp);
	return ncsdHeader.Ncsd.Signature == s_uSignature;
}

bool CNcsd::extractFile(const char* a_pFileName, n64 a_nOffset, n64 a_nSize, const char* a_pType, int a_nTypeId, bool bMediaUnitSize)
{
	bool bResult = true;
	if (a_pFileName != nullptr)
	{
		if (a_nOffset != 0 || a_nSize != 0)
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
				if (bMediaUnitSize)
				{
					a_nOffset *= m_nMediaUnitSize;
					a_nSize *= m_nMediaUnitSize;
				}
				FCopyFile(fp, m_fpNcsd, a_nOffset, a_nSize);
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			if (a_nTypeId < 0 || a_nTypeId >= 8)
			{
				printf("INFO: %s is not exists, %s will not be create\n", a_pType, a_pFileName);
			}
			else
			{
				printf("INFO: %s %d is not exists, %s will not be create\n", a_pType, a_nTypeId, a_pFileName);
			}
		}
	}
	else if ((a_nOffset != 0 || a_nSize != 0) && m_bVerbose)
	{
		if (a_nTypeId < 0 || a_nTypeId >= 8)
		{
			printf("INFO: %s is not extract\n", a_pType);
		}
		else
		{
			printf("INFO: %s %d is not extract\n", a_pType, a_nTypeId);
		}
	}
	return bResult;
}
