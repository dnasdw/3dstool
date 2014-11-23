#include "3dstool.h"
#include "ncch.h"
#include "ncsd.h"
#include "romfs.h"

C3DSTool::SOption C3DSTool::s_Option[] =
{
	{ "extract", 'x', "extract the FILE" },
	{ "create", 'c', "create the FILE" },
	{ "crypto", 'e', "decrypt/encrypt the FILE or decrypt/encrypt the part of the cxi file" },
	{ "rip", 0, "short for --rip-after-partition 7" },
	{ "rip-after-partition", 0, "rip the data after partition N[0~7]" },
	{ "pad", 0, "add the pad data behind the 3ds FILE" },
	{ "type", 't', "[[cci|ncsd|3ds]|[cxi|ncch]|romfs]\n\t\tthe type of FILE, optional" },
	{ "file", 'f', "the source of extract or the destination of create FILE, required" },
	{ "header", 0, "the header file of FILE" },
	{ "partition0", '0', "the cxi file of cci file at partition 0" },
	{ "partition1", '1', "the cxi file of cci file at partition 1" },
	{ "partition2", '2', "the cxi file of cci file at partition 2" },
	{ "partition3", '3', "the cxi file of cci file at partition 3" },
	{ "partition4", '4', "the cxi file of cci file at partition 4" },
	{ "partition5", '5', "the cxi file of cci file at partition 5" },
	{ "partition6", '6', "the cxi file of cci file at partition 6" },
	{ "partition7", '7', "the cxi file of cci file at partition 7" },
	{ "key0", 0, "short for --key 00000000000000000000000000000000" },
	{ "key", 0, "the hex string for extract/create or crypto the FILE" },
	{ "counter", 0, "the hex string for crypto the FILE by AES CTR mode" },
	{ "xor", 0, "the xor data file for crypto the FILE" },
	{ "exh-xor", 0, "short for --extendedheader-xor" },
	{ "extendedheader-xor", 0, "the xor data file for crypto the extendedheader of the cxi file" },
	{ "exefs-xor", 0, "the xor data file for crypto the exefs part of the cxi file" },
	{ "romfs-xor", 0, "the xor data file for crypto the romfs part of the cxi file" },
	{ "exh", 0, "short for --extendedheader" },
	{ "extendedheader", 0, "the extendedheader file of the cxi file" },
	{ "ace", 0, "short for --accesscontrolextended" },
	{ "accesscontrolextended", 0, "the accesscontrolextended file of the cxi file" },
	{ "logo", 0, "short for --logoregion" },
	{ "logoregion", 0, "the logoregion file of the cxi file" },
	{ "plain", 0, "short for --plainregion" },
	{ "plainregion", 0, "the plainregion file of the cxi file" },
	{ "exefs", 0, "the exefs file of the cxi file" },
	{ "romfs", 0, "the romfs file of the cxi file" },
	{ "not-update-exh-hash", 0, "short for --not-update-extendedheader-hash"},
	{ "not-update-extendedheader-hash", 0, "do not update the extendedheader hash"},
	{ "not-update-exefs-hash", 0, "do not update the exefs super block hash"},
	{ "not-update-romfs-hash", 0, "do not update the romfs super block hash"},
	{ "not-pad", 0, "do not add the pad data" },
	{ "romfs-dir", 0, "the romfs dir for the romfs file" },
	{ "verbose", 'v', "show progress" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, nullptr }
};

C3DSTool::C3DSTool()
	: m_eAction(kActionNone)
	, m_nLastPartitionIndex(7)
	, m_eFileType(kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_pHeaderFileName(nullptr)
	, m_nCryptoMode(CNcch::kCryptoModeNone)
	, m_pXorFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pRomFsXorFileName(nullptr)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pAccessControlExtendedFileName(nullptr)
	, m_pLogoRegionFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_bNotPad(false)
	, m_pRomFsDirName(nullptr)
	, m_bVerbose(false)
	, m_pMessage(nullptr)
{
	memset(m_pNcchFileName, 0, sizeof(m_pNcchFileName));
	memset(m_uKey, 0, sizeof(m_uKey));
}

C3DSTool::~C3DSTool()
{
}

int C3DSTool::ParseOptions(int a_nArgc, char* a_pArgv[])
{
	if (a_nArgc <= 1)
	{
		return 1;
	}
	for (int i = 1; i < a_nArgc; i++)
	{
		int nArgpc = static_cast<int>(strlen(a_pArgv[i]));
		int nIndex = i;
		if (a_pArgv[i][0] != '-')
		{
			for (int j = 0; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 1 && a_pArgv[i][1] != '-')
		{
			for (int j = 1; j < nArgpc; j++)
			{
				switch (parseOptions(a_pArgv[i][j], nIndex, a_nArgc, a_pArgv))
				{
				case kParseOptionReturnSuccess:
					break;
				case kParseOptionReturnIllegalOption:
					printf("ERROR: illegal option\n\n");
					return 1;
				case kParseOptionReturnNoArgument:
					printf("ERROR: no argument\n\n");
					return 1;
				case kParseOptionReturnUnknownArgument:
					printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
					return 1;
				case kParseOptionReturnOptionConflict:
					printf("ERROR: option conflict\n\n");
					return 1;
				}
			}
		}
		else if (nArgpc > 2 && a_pArgv[i][1] == '-')
		{
			switch (parseOptions(a_pArgv[i] + 2, nIndex, a_nArgc, a_pArgv))
			{
			case kParseOptionReturnSuccess:
				break;
			case kParseOptionReturnIllegalOption:
				printf("ERROR: illegal option\n\n");
				return 1;
			case kParseOptionReturnNoArgument:
				printf("ERROR: no argument\n\n");
				return 1;
			case kParseOptionReturnUnknownArgument:
				printf("ERROR: unknown argument \"%s\"\n\n", m_pMessage);
				return 1;
			case kParseOptionReturnOptionConflict:
				printf("ERROR: option conflict\n\n");
				return 1;
			}
		}
		i = nIndex;
	}
	return 0;
}

int C3DSTool::CheckOptions()
{
	if (m_eAction == kActionNone)
	{
		printf("ERROR: nothing to do\n\n");
		return 1;
	}
	if (m_eAction != kActionHelp && m_pFileName == nullptr)
	{
		printf("ERROR: no --file option\n\n");
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		switch (m_eFileType)
		{
		case C3DSTool::kFileTypeCci:
			if (m_pHeaderFileName == nullptr)
			{
				bool bNull = true;
				for (int i = 0; i < 8; i++)
				{
					if (m_pNcchFileName[i] != nullptr)
					{
						bNull = false;
					}
				}
				if (bNull)
				{
					printf("ERROR: nothing to be extract\n\n");
					return 1;
				}
			}
			break;
		case C3DSTool::kFileTypeCxi:
			if (m_pHeaderFileName == nullptr && m_pExtendedHeaderFileName == nullptr && m_pAccessControlExtendedFileName == nullptr && m_pPlainRegionFileName == nullptr && m_pExeFsFileName == nullptr && m_pRomFsFileName == nullptr)
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case C3DSTool::kFileTypeRomfs:
			if (m_pRomFsDirName == nullptr)
			{
				printf("ERROR: no --romfs-dir option\n\n");
				return 1;
			}
			break;
		default:
			break;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (m_eFileType == kFileTypeUnknown)
		{
			printf("ERROR: no --type option\n\n");
			return 1;
		}
		else if (m_eFileType == kFileTypeCxi)
		{
			if (m_pHeaderFileName == nullptr)
			{
				printf("ERROR: no --header option\n\n");
				return 1;
			}
			if (m_pAccessControlExtendedFileName != nullptr && m_pExtendedHeaderFileName == nullptr)
			{
				printf("ERROR: no --extendedheader option\n\n");
				return 1;
			}
		}
		else if (m_eFileType == kFileTypeCci)
		{
			if (m_pHeaderFileName == nullptr)
			{
				printf("ERROR: no --header option\n\n");
				return 1;
			}
			if (m_pNcchFileName[0] == nullptr)
			{
				printf("ERROR: no --partition0 option\n\n");
				return 1;
			}
		}
		else if (m_eFileType == kFileTypeRomfs)
		{
			if (m_pRomFsDirName == nullptr)
			{
				printf("ERROR: no --romfs-dir option\n\n");
				return 1;
			}
		}
	}
	if (m_eAction == kActionCrypto)
	{
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			printf("ERROR: no key or xor data file\n\n");
			return 1;
		}
		else if (m_nCryptoMode == CNcch::kCryptoModeXor)
		{
			if (m_pExtendedHeaderXorFileName != nullptr || m_pExeFsXorFileName != nullptr || m_pRomFsXorFileName != nullptr)
			{
				if (m_pXorFileName != nullptr)
				{
					printf("ERROR: --xor can not with --extendedheader-xor or --exefs-xor or --romfs-xor\n\n");
					return 1;
				}
				if (!CNcch::IsNcchFile(m_pFileName))
				{
					printf("ERROR: %s is not a ncch file\n\n", m_pFileName);
					return 1;
				}
				else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCxi && m_bVerbose)
				{
					printf("INFO: ignore --type option\n");
				}
			}
		}
	}
	if (m_eAction == kActionRip || m_eAction == kActionPad)
	{
		if (!CNcsd::IsNcsdFile(m_pFileName))
		{
			printf("ERROR: %s is not a ncsd file\n\n", m_pFileName);
			return 1;
		}
		else if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCci && m_bVerbose)
		{
			printf("INFO: ignore --type option\n");
		}
	}
	return 0;
}

int C3DSTool::Help()
{
	printf("3dstool %s by dnasdw\n\n", _3DS_TOOL_VERSION);
	printf("usage: 3dstool [option...] [FILE]...\n");
	printf("example:\n");
	printf("  3dstool -xvt0f 3ds app.cxi input.3ds --header ncsdheader.bin\n");
	printf("  3dstool -evtf cxi app.cxi --romfs-xor CTR-P-XXXX0.romfs.xorpad\n");
	printf("  3dstool -xvtf cxi app.cxi --header ncchheader.bin --exh exh.bin --ace ace.bin --plain plain.bin --exefs exefs.bin --romfs romfs.bin");
	printf("  3dstool -xvtf romfs romfs.bin --romfs-dir romfs\n");
	printf("  3dstool -cvtf romfs romfsnew.bin --romfs-dir romfs\n");
	printf("  3dstool -cvtf cxi appnew.cxi --header ncchheader.bin --exh exh.bin --ace ace.bin --plain plain.bin --exefs exefs.bin --romfs romfsnew.bin --not-update-exh-hash --not-update-exefs-hash");
	printf("  3dstool -evtf cxi appnew.cxi --romfs-xor CTR-P-XXXX0.romfs.xorpad");
	printf("  3dstool -cvt0f 3ds appnew.cxi output.3ds --header ncsdheader.bin");
	printf("\noption:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr && pOption->Doc != nullptr)
	{
		printf("  ");
		if (pOption->Key != 0)
		{
			printf("-%c,", pOption->Key);
		}
		else
		{
			printf("   ");
		}
		printf(" --%s", pOption->Name);
		if (strlen(pOption->Name) >= 8)
		{
			printf("\n\t");
		}
		printf("\t%s\n", pOption->Doc);
		pOption++;
	}
	return 0;
}

int C3DSTool::Action()
{
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			printf("ERROR: %s\n\n", m_pMessage);
			return 1;
		}
		if (!extractFile())
		{
			printf("ERROR: extract file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCreate)
	{
		if (!createFile())
		{
			printf("ERROR: create file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCrypto)
	{
		if (!cryptoFile())
		{
			printf("ERROR: crypto file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionRip)
	{
		if (!ripFile())
		{
			printf("ERROR: rip file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPad)
	{
		if (!padFile())
		{
			printf("ERROR: pad file failed\n\n");
			return 1;
		}
	}
	return 0;
}

C3DSTool::EParseOptionReturn C3DSTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "extract") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionExtract;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "create") == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionRip)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "crypto") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCrypto;
		}
		else if (m_eAction != kActionCrypto && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "rip") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionRip;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionRip && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_nLastPartitionIndex = 7;
	}
	else if (strcmp(a_pName, "rip-after-partition") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionRip;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionRip && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_nLastPartitionIndex = FSToN32(a_pArgv[++a_nIndex]);
		if (m_nLastPartitionIndex < 0 || m_nLastPartitionIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
	}
	else if (strcmp(a_pName, "pad") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPad;
		}
		else if (m_eAction != kActionPad && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		char* pType = a_pArgv[++a_nIndex];
		if (strcmp(pType, "cci") == 0 || strcmp(pType, "ncsd") == 0 || strcmp(pType, "3ds") == 0)
		{
			m_eFileType = kFileTypeCci;
		}
		else if (strcmp(pType, "cxi") == 0 || strcmp(pType, "ncch") == 0)
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (strcmp(pType, "romfs") == 0)
		{
			m_eFileType = kFileTypeRomfs;
		}
		else
		{
			m_pMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "header") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (FSStartsWith<string>(a_pName, "partition"))
	{
		int nIndex = FSToN32(a_pName + strlen("partition"));
		if (nIndex < 0 || nIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pNcchFileName[nIndex] = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "key0") == 0)
	{
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeAesCtr;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		FSHexToU8("00000000000000000000000000000000", m_uKey);
	}
	else if (strcmp(a_pName, "key") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeAesCtr;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		string sKey = a_pArgv[++a_nIndex];
		if (sKey.size() != 32 || sKey.find_first_not_of("0123456789ABCDEFabcdef") != string::npos || !FSHexToU8(sKey, m_uKey))
		{
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "counter") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeAesCtr;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeAesCtr)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_sCounter = a_pArgv[++a_nIndex];
		if (m_sCounter.size() != 32 || m_sCounter.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
		{
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeXor;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "extendedheader-xor") == 0 || strcmp(a_pName, "exh-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeXor;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pExtendedHeaderXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeXor;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pExeFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nCryptoMode == CNcch::kCryptoModeNone)
		{
			m_nCryptoMode = CNcch::kCryptoModeXor;
		}
		else if (m_nCryptoMode != CNcch::kCryptoModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pRomFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "extendedheader") == 0 || strcmp(a_pName, "exh") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExtendedHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "accesscontrolextended") == 0 || strcmp(a_pName, "ace") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pAccessControlExtendedFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "logoregion") == 0 || strcmp(a_pName, "logo") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pLogoRegionFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "plainregion") == 0 || strcmp(a_pName, "plain") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pPlainRegionFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExeFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "not-update-extendedheader-hash") == 0 || strcmp(a_pName, "not-update-exh-hash") == 0)
	{
		m_bNotUpdateExtendedHeaderHash = true;
	}
	else if (strcmp(a_pName, "not-update-exefs-hash") == 0)
	{
		m_bNotUpdateExeFsHash = true;
	}
	else if (strcmp(a_pName, "not-update-romfs-hash") == 0)
	{
		m_bNotUpdateRomFsHash = true;
	}
	else if (strcmp(a_pName, "not-pad") == 0)
	{
		m_bNotPad = true;
	}
	else if (strcmp(a_pName, "romfs-dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	return kParseOptionReturnSuccess;
}

C3DSTool::EParseOptionReturn C3DSTool::parseOptions(int a_nKey, int& a_nIndex, int m_nArgc, char* a_pArgv[])
{
	for (SOption* pOption = s_Option; pOption->Name != nullptr || pOption->Key != 0 || pOption->Doc != nullptr; pOption++)
	{
		if (pOption->Key == a_nKey)
		{
			return parseOptions(pOption->Name, a_nIndex, m_nArgc, a_pArgv);
		}
	}
	return kParseOptionReturnIllegalOption;
}

bool C3DSTool::checkFileType()
{
	if (m_eFileType == kFileTypeUnknown)
	{
		if (CNcsd::IsNcsdFile(m_pFileName))
		{
			m_eFileType = kFileTypeCci;
		}
		else if (CNcch::IsNcchFile(m_pFileName))
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (CRomFs::IsRomFsFile(m_pFileName))
		{
			m_eFileType = kFileTypeRomfs;
		}
		else
		{
			m_pMessage = "unknown file type";
			return false;
		}
	}
	else
	{
		bool bMatch = false;
		switch (m_eFileType)
		{
		case C3DSTool::kFileTypeCci:
			bMatch = CNcsd::IsNcsdFile(m_pFileName);
			break;
		case C3DSTool::kFileTypeCxi:
			bMatch = CNcch::IsNcchFile(m_pFileName);
			break;
		case C3DSTool::kFileTypeRomfs:
			bMatch = CRomFs::IsRomFsFile(m_pFileName);
			break;
		}
		if (!bMatch)
		{
			m_pMessage = "the file type is mismatch";
			return false;
		}
	}
	return true;
}

bool C3DSTool::extractFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case C3DSTool::kFileTypeCci:
	{
		CNcsd ncsd;
		ncsd.SetFileName(m_pFileName);
		ncsd.SetHeaderFileName(m_pHeaderFileName);
		ncsd.SetNcchFileName(m_pNcchFileName);
		ncsd.SetVerbose(m_bVerbose);
		bResult = ncsd.ExtractFile();
	}
		break;
	case C3DSTool::kFileTypeCxi:
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetCryptoMode(m_nCryptoMode);
		ncch.SetKey(m_uKey);
		ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		ncch.SetHeaderFileName(m_pHeaderFileName);
		ncch.SetExtendedHeaderFileName(m_pExtendedHeaderFileName);
		ncch.SetAccessControlExtendedFileName(m_pAccessControlExtendedFileName);
		ncch.SetLogoRegionFileName(m_pLogoRegionFileName);
		ncch.SetPlainRegionFileName(m_pPlainRegionFileName);
		ncch.SetExeFsFileName(m_pExeFsFileName);
		ncch.SetRomFsFileName(m_pRomFsFileName);
		ncch.SetVerbose(m_bVerbose);
		bResult = ncch.ExtractFile();
	}
		break;
	case C3DSTool::kFileTypeRomfs:
	{
		CRomFs romFs;
		romFs.SetFileName(m_pFileName);
		romFs.SetRomFsDirName(m_pRomFsDirName);
		romFs.SetVerbose(m_bVerbose);
		bResult = romFs.ExtractFile();
	}
		break;
	}
	return bResult;
}

bool C3DSTool::createFile()
{
	bool bResult = false;
	switch (m_eFileType)
	{
	case C3DSTool::kFileTypeCci:
	{
		CNcsd ncsd;
		ncsd.SetLastPartitionIndex(m_nLastPartitionIndex);
		ncsd.SetFileName(m_pFileName);
		ncsd.SetHeaderFileName(m_pHeaderFileName);
		ncsd.SetNcchFileName(m_pNcchFileName);
		ncsd.SetNotPad(m_bNotPad);
		ncsd.SetVerbose(m_bVerbose);
		bResult = ncsd.CreateFile();
	}
		break;
	case C3DSTool::kFileTypeCxi:
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetCryptoMode(m_nCryptoMode);
		ncch.SetKey(m_uKey);
		ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		ncch.SetHeaderFileName(m_pHeaderFileName);
		ncch.SetExtendedHeaderFileName(m_pExtendedHeaderFileName);
		ncch.SetAccessControlExtendedFileName(m_pAccessControlExtendedFileName);
		ncch.SetPlainRegionFileName(m_pPlainRegionFileName);
		ncch.SetExeFsFileName(m_pExeFsFileName);
		ncch.SetRomFsFileName(m_pRomFsFileName);
		ncch.SetNotUpdateExtendedHeaderHash(m_bNotUpdateExtendedHeaderHash);
		ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
		ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
		ncch.SetVerbose(m_bVerbose);
		bResult = ncch.CreateFile();
	}
		break;
	case C3DSTool::kFileTypeRomfs:
	{
		CRomFs romFs;
		romFs.SetFileName(m_pFileName);
		romFs.SetRomFsDirName(m_pRomFsDirName);
		romFs.SetVerbose(m_bVerbose);
		bResult = romFs.CreateFile();
	}
		break;
	}
	return bResult;
}

bool C3DSTool::cryptoFile()
{
	bool bResult = false;
	if (m_nCryptoMode == CNcch::kCryptoModeAesCtr && !m_sCounter.empty())
	{
		u8 uAesCtr[16] = {};
		bResult = FSHexToU8(m_sCounter, uAesCtr);
		if (bResult)
		{
			bResult = FCryptoAesCtrFile(m_pFileName, m_uKey, uAesCtr, 0, 0, true, m_bVerbose);
		}
	}
	else if (m_nCryptoMode == CNcch::kCryptoModeXor && m_pXorFileName != nullptr)
	{
		bResult = FCryptoXorFile(m_pFileName, m_pXorFileName, 0, 0, true, 0, m_bVerbose);
	}
	else
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetCryptoMode(m_nCryptoMode);
		ncch.SetKey(m_uKey);
		ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		ncch.SetVerbose(m_bVerbose);
		bResult = ncch.CryptoFile();
	}
	return bResult;
}

bool C3DSTool::ripFile()
{
	CNcsd ncsd;
	ncsd.SetLastPartitionIndex(m_nLastPartitionIndex);
	ncsd.SetFileName(m_pFileName);
	ncsd.SetVerbose(m_bVerbose);
	bool bResult = ncsd.RipFile();
	return bResult;
}

bool C3DSTool::padFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_pFileName);
	ncsd.SetVerbose(m_bVerbose);
	bool bResult = ncsd.PadFile();
	return bResult;
}

int main(int argc, char* argv[])
{
	FSetLocale();
	C3DSTool tool;
	if (tool.ParseOptions(argc, argv) != 0)
	{
		return tool.Help();
	}
	if (tool.CheckOptions() != 0)
	{
		return 1;
	}
	return tool.Action();
}
