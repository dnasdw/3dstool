#include "ncsd.h"

CNcsd::CNcsd()
	: m_pFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_bVerbose(false)
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
	FILE* fp = fopen(m_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", m_pFileName);
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, fp);
	m_nMediaUnitSize = 1LL << (m_NcsdHeader.Ncsd.Flags[6] + 9);
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
			FCopyFile(fpHeader, fp, 0, kOffsetFirstNcch);
			fclose(fpHeader);
		}
	}
	else if (m_bVerbose)
	{
		printf("INFO: ncsd header is not extract\n");
	}
	for (int i = 0; i < 8; i++)
	{
		if (m_pNcchFileName[i] != nullptr)
		{
			if (m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] != 0 || m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1] != 0)
			{
				FILE* fpNcch = fopen(m_pNcchFileName[i], "wb");
				if (fpNcch == nullptr)
				{
					printf("ERROR: create file %s failed\n\n", m_pNcchFileName[i]);
				}
				else
				{
					n64 nOffset = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] * m_nMediaUnitSize;
					n64 nSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1] * m_nMediaUnitSize;
					if (m_bVerbose)
					{
						printf("create: %s\n", m_pNcchFileName[i]);
					}
					FCopyFile(fpNcch, fp, nOffset, nSize);
					fclose(fpNcch);
				}
			}
			else if (m_bVerbose)
			{
				printf("INFO: partition %d is not exists, %s will not be create\n", i, m_pNcchFileName[i]);
			}
		}
		else if ((m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] != 0 || m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1] != 0) && m_bVerbose)
		{
			printf("INFO: partition %d is not extract\n", i);
		}
	}
	fclose(fp);
	return true;
}

bool CNcsd::IsNcsdFile(const char* a_pFileName)
{
	FILE* fp = fopen(a_pFileName, "rb");
	if (fp == nullptr)
	{
		printf("ERROR: open file %s failed\n\n", a_pFileName);
		return false;
	}
	SNcsdHeader ncsdHeader;
	fread(&ncsdHeader, sizeof(ncsdHeader), 1, fp);
	fclose(fp);
	return ncsdHeader.Ncsd.Signature == s_uSignature;
}
