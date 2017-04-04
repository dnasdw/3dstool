#include "ncsd.h"

const u32 CNcsd::s_uSignature = SDW_CONVERT_ENDIAN32('NCSD');
const n64 CNcsd::s_nOffsetFirstNcch = 0x4000;
const int CNcsd::s_nBlockSize = 0x1000;

CNcsd::CNcsd()
	: m_bVerbose(false)
	, m_bNotPad(false)
	, m_nLastPartitionIndex(7)
	, m_fpNcsd(nullptr)
	, m_nMediaUnitSize(1 << 9)
	, m_bAlignToBlockSize(false)
	, m_nValidSize(0)
{
	memset(&m_NcsdHeader, 0, sizeof(m_NcsdHeader));
	memset(&m_CardInfo, 0, sizeof(m_CardInfo));
	memset(m_nOffsetAndSize, 0, sizeof(m_nOffsetAndSize));
}

CNcsd::~CNcsd()
{
}

void CNcsd::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CNcsd::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CNcsd::SetHeaderFileName(const UString& a_sHeaderFileName)
{
	m_sHeaderFileName = a_sHeaderFileName;
}

void CNcsd::SetNcchFileName(const map<int, UString>& a_mNcchFileName)
{
	m_mNcchFileName.insert(a_mNcchFileName.begin(), a_mNcchFileName.end());
}

void CNcsd::SetNotPad(bool a_bNotPad)
{
	m_bNotPad = a_bNotPad;
}

void CNcsd::SetLastPartitionIndex(int a_nLastPartitionIndex)
{
	m_nLastPartitionIndex = a_nLastPartitionIndex;
}

void CNcsd::SetFilePtr(FILE* a_fpNcsd)
{
	m_fpNcsd = a_fpNcsd;
}

SNcsdHeader& CNcsd::GetNcsdHeader()
{
	return m_NcsdHeader;
}

n64* CNcsd::GetOffsetAndSize()
{
	return m_nOffsetAndSize;
}

bool CNcsd::ExtractFile()
{
	bool bResult = true;
	m_fpNcsd = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	calculateMediaUnitSize();
	if (!extractFile(m_sHeaderFileName, 0, s_nOffsetFirstNcch, USTR("ncsd header"), -1, false))
	{
		bResult = false;
	}
	for (int i = 0; i < 8; i++)
	{
		if (!extractFile(m_mNcchFileName[i], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2], m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1], USTR("partition"), i, true))
		{
			bResult = false;
		}
	}
	fclose(m_fpNcsd);
	return bResult;
}

bool CNcsd::CreateFile()
{
	bool bResult = true;
	m_fpNcsd = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	if (!createHeader())
	{
		fclose(m_fpNcsd);
		return false;
	}
	calculateMediaUnitSize();
	calculateAlignment();
	for (int i = 0; i < 8; i++)
	{
		if (!createNcch(i))
		{
			bResult = false;
		}
	}
	n64 nFileSize = Ftell(m_fpNcsd);
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = nFileSize;
	if (m_bNotPad && m_NcsdHeader.Ncsd.Flags[MEDIA_TYPE_INDEX] == CARD2)
	{
		m_bNotPad = false;
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: not support --not-pad with CARD2 type\n"));
		}
	}
	if (m_bNotPad)
	{
		m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>(nFileSize / m_nMediaUnitSize);
	}
	else
	{
		int nMinPower = m_NcsdHeader.Ncsd.Flags[MEDIA_TYPE_INDEX] == CARD1 ? 27 : 29;
		for (int i = nMinPower; i < 64; i++)
		{
			if (nFileSize <= 1LL << i)
			{
				m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << i) / m_nMediaUnitSize);
				break;
			}
		}
		::PadFile(m_fpNcsd, m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize - nFileSize, 0xFF);
	}
	Fseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	fclose(m_fpNcsd);
	return bResult;
}

bool CNcsd::TrimFile()
{
	m_fpNcsd = UFopen(m_sFileName.c_str(), USTR("rb+"));
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	if (m_NcsdHeader.Ncsd.Flags[MEDIA_TYPE_INDEX] == CARD2)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: not support --trim with CARD2 type\n"));
		}
		fclose(m_fpNcsd);
		return false;
	}
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	calculateMediaUnitSize();
	for (int i = m_nLastPartitionIndex + 1; i < 8; i++)
	{
		clearNcch(i);
	}
	calculateValidSize();
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = m_nValidSize;
	m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>(m_nValidSize / m_nMediaUnitSize);
	Fseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	Chsize(Fileno(m_fpNcsd), m_nValidSize);
	fclose(m_fpNcsd);
	return true;
}

bool CNcsd::PadFile()
{
	m_fpNcsd = UFopen(m_sFileName.c_str(), USTR("rb+"));
	if (m_fpNcsd == nullptr)
	{
		return false;
	}
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	if (m_NcsdHeader.Ncsd.Flags[MEDIA_TYPE_INDEX] == CARD2)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: not support --pad with CARD2 type\n"));
		}
		fclose(m_fpNcsd);
		return false;
	}
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	calculateMediaUnitSize();
	calculateValidSize();
	*reinterpret_cast<n64*>(m_CardInfo.Reserved1 + 248) = m_nValidSize;
	for (int i = 27; i < 64; i++)
	{
		if (m_nValidSize <= 1LL << i)
		{
			m_NcsdHeader.Ncsd.MediaSize = static_cast<u32>((1LL << i) / m_nMediaUnitSize);
			break;
		}
	}
	Fseek(m_fpNcsd, m_nValidSize, SEEK_SET);
	::PadFile(m_fpNcsd, m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize - m_nValidSize, 0xFF);
	Fseek(m_fpNcsd, 0, SEEK_SET);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	Chsize(Fileno(m_fpNcsd), m_NcsdHeader.Ncsd.MediaSize * m_nMediaUnitSize);
	fclose(m_fpNcsd);
	return true;
}

void CNcsd::Analyze()
{
	if (m_fpNcsd != nullptr)
	{
		n64 nFilePos = Ftell(m_fpNcsd);
		Fseek(m_fpNcsd, 0, SEEK_SET);
		fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
		calculateMediaUnitSize();
		for (int i = 0; i < 8; i++)
		{
			m_nOffsetAndSize[i * 2] = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] * m_nMediaUnitSize;
			m_nOffsetAndSize[i * 2 + 1] = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1] * m_nMediaUnitSize;
		}
		for (int i = 6; i >= 0; i--)
		{
			if (m_nOffsetAndSize[i * 2] == 0 && m_nOffsetAndSize[i * 2 + 1] == 0)
			{
				m_nOffsetAndSize[i * 2] = m_nOffsetAndSize[(i + 1) * 2];
			}
		}
		for (int i = 1; i < 8; i++)
		{
			if (m_nOffsetAndSize[i * 2] == 0 && m_nOffsetAndSize[i * 2 + 1] == 0)
			{
				m_nOffsetAndSize[i * 2] = m_nOffsetAndSize[(i - 1) * 2] + m_nOffsetAndSize[(i - 1) * 2 + 1];
			}
		}
		Fseek(m_fpNcsd, nFilePos, SEEK_SET);
	}
}

bool CNcsd::IsNcsdFile(const UString& a_sFileName)
{
	FILE* fp = UFopen(a_sFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	SNcsdHeader ncsdHeader;
	fread(&ncsdHeader, sizeof(ncsdHeader), 1, fp);
	fclose(fp);
	return ncsdHeader.Ncsd.Signature == s_uSignature;
}

void CNcsd::calculateMediaUnitSize()
{
	m_nMediaUnitSize = 1LL << (m_NcsdHeader.Ncsd.Flags[MEDIA_UNIT_SIZE] + 9);
}

void CNcsd::calculateAlignment()
{
	m_bAlignToBlockSize = true;
	for (int i = 0; m_bAlignToBlockSize && i < 8; i++)
	{
		m_bAlignToBlockSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] % 8 == 0 && m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1] % 8 == 0;
	}
}

void CNcsd::calculateValidSize()
{
	m_nValidSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[0] + m_NcsdHeader.Ncsd.ParitionOffsetAndSize[1];
	if (m_nValidSize < s_nOffsetFirstNcch / m_nMediaUnitSize)
	{
		m_nValidSize = s_nOffsetFirstNcch / m_nMediaUnitSize;
	}
	for (int i = 1; i < 8; i++)
	{
		n64 nSize = m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2] + m_NcsdHeader.Ncsd.ParitionOffsetAndSize[i * 2 + 1];
		if (nSize > m_nValidSize)
		{
			m_nValidSize = nSize;
		}
	}
	m_nValidSize *= m_nMediaUnitSize;
}

bool CNcsd::extractFile(const UString& a_sFileName, n64 a_nOffset, n64 a_nSize, const UChar* a_pType, int a_nTypeId, bool bMediaUnitSize)
{
	bool bResult = true;
	if (!a_sFileName.empty())
	{
		if (a_nOffset != 0 || a_nSize != 0)
		{
			FILE* fp = UFopen(a_sFileName.c_str(), USTR("wb"));
			if (fp == nullptr)
			{
				bResult = false;
			}
			else
			{
				if (m_bVerbose)
				{
					UPrintf(USTR("save: %") PRIUS USTR("\n"), a_sFileName.c_str());
				}
				if (bMediaUnitSize)
				{
					a_nOffset *= m_nMediaUnitSize;
					a_nSize *= m_nMediaUnitSize;
				}
				CopyFile(fp, m_fpNcsd, a_nOffset, a_nSize);
				fclose(fp);
			}
		}
		else if (m_bVerbose)
		{
			if (a_nTypeId < 0 || a_nTypeId >= 8)
			{
				UPrintf(USTR("INFO: %") PRIUS USTR(" is not exists, %") PRIUS USTR(" will not be create\n"), a_pType, a_sFileName.c_str());
			}
			else
			{
				UPrintf(USTR("INFO: %") PRIUS USTR(" %d is not exists, %") PRIUS USTR(" will not be create\n"), a_pType, a_nTypeId, a_sFileName.c_str());
			}
		}
	}
	else if ((a_nOffset != 0 || a_nSize != 0) && m_bVerbose)
	{
		if (a_nTypeId < 0 || a_nTypeId >= 8)
		{
			UPrintf(USTR("INFO: %") PRIUS USTR(" is not extract\n"), a_pType);
		}
		else
		{
			UPrintf(USTR("INFO: %") PRIUS USTR(" %d is not extract\n"), a_pType, a_nTypeId);
		}
	}
	return bResult;
}

bool CNcsd::createHeader()
{
	FILE* fp = UFopen(m_sHeaderFileName.c_str(), USTR("rb"));
	if (fp == nullptr)
	{
		return false;
	}
	Fseek(fp, 0, SEEK_END);
	n64 nFileSize = Ftell(fp);
	if (nFileSize < sizeof(m_NcsdHeader) + sizeof(m_CardInfo))
	{
		fclose(fp);
		UPrintf(USTR("ERROR: ncsd header is too short\n\n"));
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("load: %") PRIUS USTR("\n"), m_sHeaderFileName.c_str());
	}
	Fseek(fp, 0, SEEK_SET);
	fread(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, fp);
	fread(&m_CardInfo, sizeof(m_CardInfo), 1, fp);
	fclose(fp);
	fwrite(&m_NcsdHeader, sizeof(m_NcsdHeader), 1, m_fpNcsd);
	fwrite(&m_CardInfo, sizeof(m_CardInfo), 1, m_fpNcsd);
	::PadFile(m_fpNcsd, s_nOffsetFirstNcch - Ftell(m_fpNcsd), 0xFF);
	return true;
}

bool CNcsd::createNcch(int a_nIndex)
{
	if (!m_mNcchFileName[a_nIndex].empty())
	{
		FILE* fp = UFopen(m_mNcchFileName[a_nIndex].c_str(), USTR("rb"));
		if (fp == nullptr)
		{
			clearNcch(a_nIndex);
			return false;
		}
		if (m_bVerbose)
		{
			UPrintf(USTR("load: %") PRIUS USTR("\n"), m_mNcchFileName[a_nIndex].c_str());
		}
		Fseek(fp, 0, SEEK_END);
		n64 nFileSize = Ftell(fp);
		if (a_nIndex == 0)
		{
			if (nFileSize < sizeof(SNcchHeader))
			{
				fclose(fp);
				clearNcch(a_nIndex);
				return false;
			}
			Fseek(fp, sizeof(SNcchHeader) - sizeof(NcchCommonHeaderStruct), SEEK_SET);
			fread(&m_CardInfo.NcchHeader, 1, sizeof(m_CardInfo.NcchHeader), fp);
		}
		Fseek(fp, 0, SEEK_SET);
		m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2] = static_cast<u32>(Ftell(m_fpNcsd) / m_nMediaUnitSize);
		m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2 + 1] = static_cast<u32>(Align(nFileSize, m_bAlignToBlockSize ? s_nBlockSize : m_nMediaUnitSize) / m_nMediaUnitSize);
		CopyFile(m_fpNcsd, fp, 0, nFileSize);
		fclose(fp);
		::PadFile(m_fpNcsd, Align(Ftell(m_fpNcsd), m_bAlignToBlockSize ? s_nBlockSize : m_nMediaUnitSize) - Ftell(m_fpNcsd), 0);
	}
	else
	{
		clearNcch(a_nIndex);
	}
	return true;
}

void CNcsd::clearNcch(int a_nIndex)
{
	m_NcsdHeader.Ncsd.PartitionFsType[a_nIndex] = FS_TYPE_DEFAULT;
	m_NcsdHeader.Ncsd.PartitionCryptType[a_nIndex] = ENCRYPTO_TYPE_DEFAULT;
	m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2] = 0;
	m_NcsdHeader.Ncsd.ParitionOffsetAndSize[a_nIndex * 2 + 1] = 0;
	memset(&m_NcsdHeader.Ncsd.PartitionId[a_nIndex], 0, sizeof(m_NcsdHeader.Ncsd.PartitionId[a_nIndex]));
	if (a_nIndex == 0)
	{
		memset(&m_CardInfo.NcchHeader, 0, sizeof(m_CardInfo.NcchHeader));
	}
}
