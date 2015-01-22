#include "3dstool.h"
#include "backwardlz77.h"
#include "banner.h"
#include "exefs.h"
#include "lz77.h"
#include "ncch.h"
#include "ncsd.h"
#include "patch.h"
#include "romfs.h"

C3DSTool::SOption C3DSTool::s_Option[] =
{
	{ nullptr, 0, "action:" },
	{ "extract", 'x', "extract the target file" },
	{ "create", 'c', "create the target file" },
	{ "encrypt", 'e', "encrypt the target file" },
	{ "uncompress", 'u', "uncompress the target file" },
	{ "compress", 'z', "compress the target file" },
	{ "trim", 'r', "trim the cci file" },
	{ "pad", 'p', "pad the cci file" },
	{ "diff", 0, "create the patch file from the old file and the new file" },
	{ "patch", 0, "apply the patch file to the target file"},
	{ "sample", 0, "show the samples" },
	{ "help", 'h', "show this help" },
	{ nullptr, 0, "\ncommon:" },
	{ "type", 't', "[[card|cci|3ds]|[nand|exec|cxi]|[data|cfa]|exefs|romfs|banner]\n\t\tthe type of the file, optional" },
	{ "file", 'f', "the target file, required" },
	{ "verbose", 'v', "show the info" },
	{ nullptr, 0, " cci/cxi/cfa/exefs:" },
	{ nullptr, 0, "  extract/create:" },
	{ "header", 0, "the header file of the target file" },
	{ nullptr, 0, " encrypt:" },
	{ "key0", 0, "short for --key 00000000000000000000000000000000" },
	{ "key", 0, "the hex string of the key used by the AES-CTR encryption" },
	{ "counter", 0, "the hex string of the counter used by the AES-CTR encryption" },
	{ "xor", 0, "the xor data file used by the xor encryption" },
	{ nullptr, 0, " uncompress/compress:" },
	{ "compress-type" , 0, "[blz|lz(ex)]\n\t\tthe type of the compress" },
	{ "compress-out", 0, "the output file of uncompressed or compressed" },
	{ nullptr, 0, " diff:" },
	{ "old", 0, "the old file" },
	{ "new", 0, "the new file" },
	{ nullptr, 0, "  patch:" },
	{ "patch-file", 0, "the patch file" },
	{ nullptr, 0, "\ncci:" },
	{ nullptr, 0, " extract/create:" },
	{ "partition0", '0', "the cxi file of the cci file at partition 0" },
	{ "partition1", '1', "the cfa file of the cci file at partition 1" },
	{ "partition2", '2', "the cfa file of the cci file at partition 2" },
	{ "partition3", '3', "the cfa file of the cci file at partition 3" },
	{ "partition4", '4', "the cfa file of the cci file at partition 4" },
	{ "partition5", '5', "the cfa file of the cci file at partition 5" },
	{ "partition6", '6', "the cfa file of the cci file at partition 6" },
	{ "partition7", '7', "the cfa file of the cci file at partition 7" },
	{ nullptr, 0, " trim:" },
	{ "trim-after-partition", 0, "[0~7], the index of the last reserve partition, optional" },
	{ nullptr, 0, "\ncxi:" },
	{ nullptr, 0, " create:" },
	{ "not-update-exh-hash", 0, nullptr },
	{ "not-update-extendedheader-hash", 0, "do not update the extendedheader hash" },
	{ "not-update-exefs-hash", 0, "do not update the exefs super block hash" },
	{ "not-update-romfs-hash", 0, "do not update the romfs super block hash" },
	{ "not-pad", 0, "do not add the pad data" },
	{ nullptr, 0, "  extract:" },
	{ "exh", 0, nullptr },
	{ "extendedheader", 0, "the extendedheader file of the cxi file" },
	{ "logo", 0, nullptr },
	{ "logoregion", 0, "the logoregion file of the cxi file" },
	{ "plain", 0, nullptr },
	{ "plainregion", 0, "the plainregion file of the cxi file" },
	{ "exefs", 0, "the exefs file of the cxi file" },
	{ nullptr, 0, "   encrypt:" },
	{ "exh-xor", 0, nullptr },
	{ "extendedheader-xor", 0, "the xor data file used by encrypt the extendedheader of the cxi file" },
	{ "exefs-xor", 0, "the xor data file used by encrypt the exefs of the cxi file" },
	{ "exefs-top-xor", 0, "the xor data file used by encrypt the top section of the exefs of the cxi file" },
	{ nullptr, 0, " cfa:" },
	{ nullptr, 0, "  extract/create:" },
	{ "romfs", 0, "the romfs file of the cxi/cfa file" },
	{ nullptr, 0, "   encrypt:" },
	{ "romfs-xor", 0, "the xor data file used by encrypt the romfs of the cxi/cfa file" },
	{ nullptr, 0, "\nexefs:" },
	{ nullptr, 0, " extract/create:" },
	{ "exefs-dir", 0, "the exefs dir for the exefs file" },
	{ nullptr, 0, "\nromfs:" },
	{ nullptr, 0, " extract/create:" },
	{ "romfs-dir", 0, "the romfs dir for the romfs file" },
	{ nullptr, 0, "\nbanner:" },
	{ nullptr, 0, " extract/create:" },
	{ "banner-dir", 0, "the banner dir for the banner file"},
	{ nullptr, 0, nullptr }
};

C3DSTool::C3DSTool()
	: m_eAction(kActionNone)
	, m_eFileType(kFileTypeUnknown)
	, m_pFileName(nullptr)
	, m_bVerbose(false)
	, m_pHeaderFileName(nullptr)
	, m_nEncryptMode(CNcch::kEncryptModeNone)
	, m_pXorFileName(nullptr)
	, m_eCompressType(kCompressTypeNone)
	, m_pCompressOutFileName(nullptr)
	, m_pOldFileName(nullptr)
	, m_pNewFileName(nullptr)
	, m_pPatchFileName(nullptr)
	, m_nLastPartitionIndex(7)
	, m_bNotUpdateExtendedHeaderHash(false)
	, m_bNotUpdateExeFsHash(false)
	, m_bNotUpdateRomFsHash(false)
	, m_bNotPad(false)
	, m_pExtendedHeaderFileName(nullptr)
	, m_pLogoRegionFileName(nullptr)
	, m_pPlainRegionFileName(nullptr)
	, m_pExeFsFileName(nullptr)
	, m_pRomFsFileName(nullptr)
	, m_pExtendedHeaderXorFileName(nullptr)
	, m_pExeFsXorFileName(nullptr)
	, m_pExeFsTopXorFileName(nullptr)
	, m_pRomFsXorFileName(nullptr)
	, m_pExeFsDirName(nullptr)
	, m_pRomFsDirName(nullptr)
	, m_pBannerDirName(nullptr)
	, m_bUncompress(false)
	, m_bCompress(false)
	, m_pMessage(nullptr)
{
	memset(m_uKey, 0, sizeof(m_uKey));
	memset(m_pNcchFileName, 0, sizeof(m_pNcchFileName));
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
			printf("ERROR: illegal option\n\n");
			return 1;
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
	if (m_eAction != kActionDiff && m_eAction != kActionSample && m_eAction != kActionHelp && m_pFileName == nullptr)
	{
		printf("ERROR: no --file option\n\n");
		return 1;
	}
	if (m_eAction == kActionExtract)
	{
		if (!checkFileType())
		{
			printf("ERROR: %s\n\n", m_pMessage);
			return 1;
		}
		switch (m_eFileType)
		{
		case kFileTypeCci:
			if (m_pHeaderFileName == nullptr)
			{
				bool bNull = true;
				for (int i = 0; i < 8; i++)
				{
					if (m_pNcchFileName[i] != nullptr)
					{
						bNull = false;
						break;
					}
				}
				if (bNull)
				{
					printf("ERROR: nothing to be extract\n\n");
					return 1;
				}
			}
			break;
		case kFileTypeCxi:
			if (m_pHeaderFileName == nullptr && m_pExtendedHeaderFileName == nullptr && m_pLogoRegionFileName == nullptr && m_pPlainRegionFileName == nullptr && m_pExeFsFileName == nullptr && m_pRomFsFileName == nullptr)
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeCfa:
			if (m_pHeaderFileName == nullptr && m_pRomFsFileName == nullptr)
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeExefs:
			if (m_pHeaderFileName == nullptr && m_pExeFsDirName == nullptr)
			{
				printf("ERROR: nothing to be extract\n\n");
				return 1;
			}
			break;
		case kFileTypeRomfs:
			if (m_pRomFsDirName == nullptr)
			{
				printf("ERROR: no --romfs-dir option\n\n");
				return 1;
			}
			break;
		case kFileTypeBanner:
			if (m_pBannerDirName == nullptr)
			{
				printf("ERROR: no --banner-dir option\n\n");
				return 1;
			}
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
		else
		{
			if (m_eFileType == kFileTypeCci || m_eFileType == kFileTypeCxi || m_eFileType == kFileTypeCfa || m_eFileType == kFileTypeExefs)
			{
				if (m_pHeaderFileName == nullptr)
				{
					printf("ERROR: no --header option\n\n");
					return 1;
				}
			}
			if (m_eFileType == kFileTypeCci)
			{
				if (m_pNcchFileName[0] == nullptr)
				{
					printf("ERROR: no --partition0 option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCxi)
			{
				if (m_pExtendedHeaderFileName == nullptr || m_pExeFsFileName == nullptr)
				{
					printf("ERROR: no --extendedheader or --exefs option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeCfa)
			{
				if (m_pRomFsFileName == nullptr)
				{
					printf("ERROR: no --romfs option\n\n");
					return 1;
				}
			}
			else if (m_eFileType == kFileTypeExefs)
			{
				if (m_pExeFsDirName == nullptr)
				{
					printf("ERROR: no --exefs-dir option\n\n");
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
			else if (m_eFileType == kFileTypeBanner)
			{
				if (m_pBannerDirName == nullptr)
				{
					printf("ERROR: no --banner-dir option\n\n");
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionEncrypt)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			printf("ERROR: no key or xor data file\n\n");
			return 1;
		}
		else if (m_nEncryptMode == CNcch::kEncryptModeXor)
		{
			if (m_pExtendedHeaderXorFileName != nullptr || m_pExeFsXorFileName != nullptr || m_pRomFsXorFileName != nullptr)
			{
				if (m_pXorFileName != nullptr)
				{
					printf("ERROR: --xor can not with --extendedheader-xor or --exefs-xor or --romfs-xor\n\n");
					return 1;
				}
				if (CNcch::IsCxiFile(m_pFileName))
				{
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCxi && m_bVerbose)
					{
						printf("INFO: ignore --type option\n");
					}
				}
				else if (CNcch::IsCfaFile(m_pFileName))
				{
					if (m_pRomFsXorFileName == nullptr)
					{
						printf("ERROR: no --romfs-xor option\n\n");
						return 1;
					}
					if (m_eFileType != kFileTypeUnknown && m_eFileType != kFileTypeCfa && m_bVerbose)
					{
						printf("INFO: ignore --type option\n");
					}
				}
				else
				{
					printf("ERROR: %s is not a ncch file\n\n", m_pFileName);
					return 1;
				}
			}
		}
	}
	if (m_eAction == kActionUncompress || m_eAction == kActionCompress)
	{
		if (m_eCompressType == kCompressTypeNone)
		{
			printf("ERROR: no --compress-type option\n\n");
			return 1;
		}
		if (m_pCompressOutFileName == nullptr)
		{
			m_pCompressOutFileName = m_pFileName;
		}
	}
	if (m_eAction == kActionTrim || m_eAction == kActionPad)
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
	if (m_eAction == kActionDiff)
	{
		if (m_pOldFileName == nullptr)
		{
			printf("ERROR: no --old option\n\n");
			return 1;
		}
		if (m_pNewFileName == nullptr)
		{
			printf("ERROR: no --new option\n\n");
			return 1;
		}
		if (m_pPatchFileName == nullptr)
		{
			printf("ERROR: no --patch-file option\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (m_pPatchFileName == nullptr)
		{
			printf("ERROR: no --patch-file option\n\n");
			return 1;
		}
	}
	return 0;
}

int C3DSTool::Help()
{
	printf("3dstool %s by dnasdw\n\n", _3DS_TOOL_VERSION);
	printf("usage: 3dstool [option...] [option]...\n\n");
	printf("option:\n");
	SOption* pOption = s_Option;
	while (pOption->Name != nullptr || pOption->Doc != nullptr)
	{
		if (pOption->Name != nullptr)
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
			printf(" --%-8s", pOption->Name);
			if (strlen(pOption->Name) >= 8 && pOption->Doc != nullptr)
			{
				printf("\n%16s", "");
			}
		}
		if (pOption->Doc != nullptr)
		{
			printf("%s", pOption->Doc);
		}
		printf("\n");
		pOption++;
	}
	return 0;
}

int C3DSTool::Action()
{
	if (m_eAction == kActionExtract)
	{
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
	if (m_eAction == kActionEncrypt)
	{
		if (!encryptFile())
		{
			printf("ERROR: encrypt file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionUncompress)
	{
		if (!uncompressFile())
		{
			printf("ERROR: uncompress file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionCompress)
	{
		if (!compressFile())
		{
			printf("ERROR: compress file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionTrim)
	{
		if (!trimFile())
		{
			printf("ERROR: trim file failed\n\n");
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
	if (m_eAction == kActionDiff)
	{
		if (!diffFile())
		{
			printf("ERROR: create patch file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionPatch)
	{
		if (!patchFile())
		{
			printf("ERROR: apply patch file failed\n\n");
			return 1;
		}
	}
	if (m_eAction == kActionSample)
	{
		return sample();
	}
	if (m_eAction == kActionHelp)
	{
		return Help();
	}
	return 0;
}

C3DSTool::EParseOptionReturn C3DSTool::parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[])
{
	if (strcmp(a_pName, "extract") == 0)
	{
		if (m_eAction == kActionNone || m_eAction == kActionUncompress)
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
		if (m_eAction == kActionNone || m_eAction == kActionCompress)
		{
			m_eAction = kActionCreate;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "encrypt") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionEncrypt;
		}
		else if (m_eAction != kActionEncrypt && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "uncompress") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionUncompress;
		}
		else if (m_eAction != kActionExtract && m_eAction != kActionUncompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bUncompress = true;
	}
	else if (strcmp(a_pName, "compress") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionCompress;
		}
		else if (m_eAction != kActionCreate && m_eAction != kActionCompress && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_bCompress = true;
	}
	else if (strcmp(a_pName, "trim") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionTrim;
		}
		else if (m_eAction != kActionTrim && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
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
	else if (strcmp(a_pName, "diff") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionDiff;
		}
		else if (m_eAction != kActionDiff && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "patch") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionPatch;
		}
		else if (m_eAction != kActionPatch && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "sample") == 0)
	{
		if (m_eAction == kActionNone)
		{
			m_eAction = kActionSample;
		}
		else if (m_eAction != kActionSample && m_eAction != kActionHelp)
		{
			return kParseOptionReturnOptionConflict;
		}
	}
	else if (strcmp(a_pName, "help") == 0)
	{
		m_eAction = kActionHelp;
	}
	else if (strcmp(a_pName, "type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		char* pType = a_pArgv[++a_nIndex];
		if (strcmp(pType, "card") == 0 || strcmp(pType, "cci") == 0 || strcmp(pType, "3ds") == 0)
		{
			m_eFileType = kFileTypeCci;
		}
		else if (strcmp(pType, "nand") == 0 || strcmp(pType, "exec") == 0 || strcmp(pType, "cxi") == 0)
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (strcmp(pType, "data") == 0 || strcmp(pType, "cfa") == 0)
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (strcmp(pType, "exefs") == 0)
		{
			m_eFileType = kFileTypeExefs;
		}
		else if (strcmp(pType, "romfs") == 0)
		{
			m_eFileType = kFileTypeRomfs;
		}
		else if (strcmp(pType, "banner") == 0)
		{
			m_eFileType = kFileTypeBanner;
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
	else if (strcmp(a_pName, "verbose") == 0)
	{
		m_bVerbose = true;
	}
	else if (strcmp(a_pName, "header") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pHeaderFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "key0") == 0)
	{
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
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
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
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
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeAesCtr;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeAesCtr)
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
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "compress-type") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		char* pType = a_pArgv[++a_nIndex];
		if (strcmp(pType, "blz") == 0)
		{
			m_eCompressType = kCompressTypeBLZ;
		}
		else if (strcmp(pType, "lz") == 0)
		{
			m_eCompressType = kCompressTypeLZ;
		}
		else if (strcmp(pType, "lzex") == 0)
		{
			m_eCompressType = kCompressTypeLZEx;
		}
		else
		{
			m_pMessage = pType;
			return kParseOptionReturnUnknownArgument;
		}
	}
	else if (strcmp(a_pName, "compress-out") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pCompressOutFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "old") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pOldFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "new") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pNewFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "patch-file") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pPatchFileName = a_pArgv[++a_nIndex];
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
	else if (strcmp(a_pName, "trim-after-partition") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_nLastPartitionIndex = FSToN32(a_pArgv[++a_nIndex]);
		if (m_nLastPartitionIndex < 0 || m_nLastPartitionIndex >= 8)
		{
			return kParseOptionReturnIllegalOption;
		}
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
	else if (strcmp(a_pName, "extendedheader") == 0 || strcmp(a_pName, "exh") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExtendedHeaderFileName = a_pArgv[++a_nIndex];
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
	else if (strcmp(a_pName, "extendedheader-xor") == 0 || strcmp(a_pName, "exh-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
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
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pExeFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-top-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pExeFsTopXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs-xor") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		if (m_nEncryptMode == CNcch::kEncryptModeNone)
		{
			m_nEncryptMode = CNcch::kEncryptModeXor;
		}
		else if (m_nEncryptMode != CNcch::kEncryptModeXor)
		{
			return kParseOptionReturnOptionConflict;
		}
		m_pRomFsXorFileName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "exefs-dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pExeFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "romfs-dir") == 0)
	{
		if (a_nIndex + 1 >= a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pRomFsDirName = a_pArgv[++a_nIndex];
	}
	else if (strcmp(a_pName, "banner-dir") == 0)
	{
		if (a_nIndex + 1 > a_nArgc)
		{
			return kParseOptionReturnNoArgument;
		}
		m_pBannerDirName = a_pArgv[++a_nIndex];
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
		else if (CNcch::IsCxiFile(m_pFileName))
		{
			m_eFileType = kFileTypeCxi;
		}
		else if (CNcch::IsCfaFile(m_pFileName))
		{
			m_eFileType = kFileTypeCfa;
		}
		else if (CExeFs::IsExeFsFile(m_pFileName, 0))
		{
			m_eFileType = kFileTypeExefs;
		}
		else if (CRomFs::IsRomFsFile(m_pFileName))
		{
			m_eFileType = kFileTypeRomfs;
		}
		else if (CBanner::IsBannerFile(m_pFileName))
		{
			m_eFileType = kFileTypeBanner;
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
		case kFileTypeCci:
			bMatch = CNcsd::IsNcsdFile(m_pFileName);
			break;
		case kFileTypeCxi:
			bMatch = CNcch::IsCxiFile(m_pFileName);
			break;
		case kFileTypeCfa:
			bMatch = CNcch::IsCfaFile(m_pFileName);
			break;
		case kFileTypeExefs:
			bMatch = CExeFs::IsExeFsFile(m_pFileName, 0);
			break;
		case kFileTypeRomfs:
			bMatch = CRomFs::IsRomFsFile(m_pFileName);
			break;
		case kFileTypeBanner:
			bMatch = CBanner::IsBannerFile(m_pFileName);
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
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_pFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_pHeaderFileName);
			ncsd.SetNcchFileName(m_pNcchFileName);
			bResult = ncsd.ExtractFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_uKey);
			ncch.SetExtendedHeaderFileName(m_pExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_pLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_pPlainRegionFileName);
			ncch.SetExeFsFileName(m_pExeFsFileName);
			ncch.SetRomFsFileName(m_pRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_pExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_uKey);
			ncch.SetRomFsFileName(m_pRomFsFileName);
			ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
			bResult = ncch.ExtractFile();
		}
		break;
	case kFileTypeExefs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_pFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_pHeaderFileName);
			exeFs.SetExeFsDirName(m_pExeFsDirName);
			exeFs.SetUncompress(m_bUncompress);
			bResult = exeFs.ExtractFile();
		}
		break;
	case kFileTypeRomfs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_pFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_pRomFsDirName);
			bResult = romFs.ExtractFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_pFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_pBannerDirName);
			banner.SetUncompress(m_bUncompress);
			bResult = banner.ExtractFile();
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
	case kFileTypeCci:
		{
			CNcsd ncsd;
			ncsd.SetFileName(m_pFileName);
			ncsd.SetVerbose(m_bVerbose);
			ncsd.SetHeaderFileName(m_pHeaderFileName);
			ncsd.SetNcchFileName(m_pNcchFileName);
			ncsd.SetNotPad(m_bNotPad);
			bResult = ncsd.CreateFile();
		}
		break;
	case kFileTypeCxi:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_uKey);
			ncch.SetNotUpdateExtendedHeaderHash(m_bNotUpdateExtendedHeaderHash);
			ncch.SetNotUpdateExeFsHash(m_bNotUpdateExeFsHash);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetExtendedHeaderFileName(m_pExtendedHeaderFileName);
			ncch.SetLogoRegionFileName(m_pLogoRegionFileName);
			ncch.SetPlainRegionFileName(m_pPlainRegionFileName);
			ncch.SetExeFsFileName(m_pExeFsFileName);
			ncch.SetRomFsFileName(m_pRomFsFileName);
			ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
			ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
			ncch.SetExeFsTopXorFileName(m_pExeFsTopXorFileName);
			ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeCfa:
		{
			CNcch ncch;
			ncch.SetFileName(m_pFileName);
			ncch.SetVerbose(m_bVerbose);
			ncch.SetHeaderFileName(m_pHeaderFileName);
			ncch.SetEncryptMode(m_nEncryptMode);
			ncch.SetKey(m_uKey);
			ncch.SetNotUpdateRomFsHash(m_bNotUpdateRomFsHash);
			ncch.SetRomFsFileName(m_pRomFsFileName);
			ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
			bResult = ncch.CreateFile();
		}
		break;
	case kFileTypeExefs:
		{
			CExeFs exeFs;
			exeFs.SetFileName(m_pFileName);
			exeFs.SetVerbose(m_bVerbose);
			exeFs.SetHeaderFileName(m_pHeaderFileName);
			exeFs.SetExeFsDirName(m_pExeFsDirName);
			exeFs.SetCompress(m_bCompress);
			bResult = exeFs.CreateFile();
		}
		break;
	case kFileTypeRomfs:
		{
			CRomFs romFs;
			romFs.SetFileName(m_pFileName);
			romFs.SetVerbose(m_bVerbose);
			romFs.SetRomFsDirName(m_pRomFsDirName);
			romFs.SetRomFsFileName(m_pRomFsFileName);
			bResult = romFs.CreateFile();
		}
		break;
	case kFileTypeBanner:
		{
			CBanner banner;
			banner.SetFileName(m_pFileName);
			banner.SetVerbose(m_bVerbose);
			banner.SetBannerDirName(m_pBannerDirName);
			banner.SetCompress(m_bCompress);
			bResult = banner.CreateFile();
		}
	}
	return bResult;
}

bool C3DSTool::encryptFile()
{
	bool bResult = false;
	if (m_nEncryptMode == CNcch::kEncryptModeAesCtr && !m_sCounter.empty())
	{
		u8 uAesCtr[16] = {};
		bResult = FSHexToU8(m_sCounter, uAesCtr);
		if (bResult)
		{
			bResult = FEncryptAesCtrFile(m_pFileName, m_uKey, uAesCtr, 0, 0, true, m_bVerbose);
		}
	}
	else if (m_nEncryptMode == CNcch::kEncryptModeXor && m_pXorFileName != nullptr)
	{
		bResult = FEncryptXorFile(m_pFileName, m_pXorFileName, 0, 0, true, 0, m_bVerbose);
	}
	else if (CNcch::IsCxiFile(m_pFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_uKey);
		ncch.SetExtendedHeaderXorFileName(m_pExtendedHeaderXorFileName);
		ncch.SetExeFsXorFileName(m_pExeFsXorFileName);
		ncch.SetExeFsTopXorFileName(m_pExeFsTopXorFileName);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		bResult = ncch.EncryptFile();
	}
	else if (CNcch::IsCfaFile(m_pFileName))
	{
		CNcch ncch;
		ncch.SetFileName(m_pFileName);
		ncch.SetVerbose(m_bVerbose);
		ncch.SetEncryptMode(m_nEncryptMode);
		ncch.SetKey(m_uKey);
		ncch.SetRomFsXorFileName(m_pRomFsXorFileName);
		bResult = ncch.EncryptFile();
	}
	return bResult;
}

bool C3DSTool::uncompressFile()
{
	FILE* fp = FFopen(m_pFileName, "rb");
	bool bResult = fp != nullptr;
	if (bResult)
	{
		FFseek(fp, 0, SEEK_END);
		u32 uCompressedSize = static_cast<u32>(FFtell(fp));
		FFseek(fp, 0, SEEK_SET);
		u8* pCompressed = new u8[uCompressedSize];
		fread(pCompressed, 1, uCompressedSize, fp);
		fclose(fp);
		u32 uUncompressedSize = 0;
		switch (m_eCompressType)
		{
		case C3DSTool::kCompressTypeBLZ:
			bResult = CBackwardLZ77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		case C3DSTool::kCompressTypeLZ:
		case C3DSTool::kCompressTypeLZEx:
			bResult = CLZ77::GetUncompressedSize(pCompressed, uCompressedSize, uUncompressedSize);
			break;
		}
		if (bResult)
		{
			u8* pUncompressed = new u8[uUncompressedSize];
			switch (m_eCompressType)
			{
			case C3DSTool::kCompressTypeBLZ:
				bResult = CBackwardLZ77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			case C3DSTool::kCompressTypeLZ:
			case C3DSTool::kCompressTypeLZEx:
				bResult = CLZ77::Uncompress(pCompressed, uCompressedSize, pUncompressed, uUncompressedSize);
				break;
			}
			if (bResult)
			{
				fp = FFopen(m_pCompressOutFileName, "wb");
				bResult = fp != nullptr;
				if (bResult)
				{
					fwrite(pUncompressed, 1, uUncompressedSize, fp);
					fclose(fp);
				}
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
	return bResult;
}

bool C3DSTool::compressFile()
{
	FILE* fp = FFopen(m_pFileName, "rb");
	bool bReuslt = fp != nullptr;
	if (bReuslt)
	{
		FFseek(fp, 0, SEEK_END);
		u32 uUncompressedSize = static_cast<u32>(FFtell(fp));
		FFseek(fp, 0, SEEK_SET);
		u8* pUncompressed = new u8[uUncompressedSize];
		fread(pUncompressed, 1, uUncompressedSize, fp);
		fclose(fp);
		u32 uCompressedSize = 0;
		switch (m_eCompressType)
		{
		case C3DSTool::kCompressTypeBLZ:
			uCompressedSize = uUncompressedSize;
			break;
		case C3DSTool::kCompressTypeLZ:
		case C3DSTool::kCompressTypeLZEx:
			uCompressedSize = CLZ77::GetCompressBoundSize(uUncompressedSize);
			break;
		}
		u8* pCompressed = new u8[uCompressedSize];
		switch (m_eCompressType)
		{
		case C3DSTool::kCompressTypeBLZ:
			bReuslt = CBackwardLZ77::Compress(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize);
			break;
		case C3DSTool::kCompressTypeLZ:
			bReuslt = CLZ77::CompressLZ(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize);
			break;
		case C3DSTool::kCompressTypeLZEx:
			bReuslt = CLZ77::CompressLZEx(pUncompressed, uUncompressedSize, pCompressed, uCompressedSize);
			break;
		}
		if (bReuslt)
		{
			fp = FFopen(m_pCompressOutFileName, "wb");
			bReuslt = fp != nullptr;
			if (bReuslt)
			{
				fwrite(pCompressed, 1, uCompressedSize, fp);
				fclose(fp);
			}
		}
		else
		{
			printf("ERROR: compress error\n\n");
		}
		delete[] pCompressed;
		delete[] pUncompressed;
	}
	return bReuslt;
}

bool C3DSTool::trimFile()
{
	CNcsd ncsd;
	ncsd.SetFileName(m_pFileName);
	ncsd.SetVerbose(m_bVerbose);
	ncsd.SetLastPartitionIndex(m_nLastPartitionIndex);
	bool bResult = ncsd.TrimFile();
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

bool C3DSTool::diffFile()
{
	CPatch patch;
	patch.SetFileType(m_eFileType);
	patch.SetVerbose(m_bVerbose);
	patch.SetOldFileName(m_pOldFileName);
	patch.SetNewFileName(m_pNewFileName);
	patch.SetPatchFileName(m_pPatchFileName);
	return patch.CreatePatchFile();
}

bool C3DSTool::patchFile()
{
	CPatch patch;
	patch.SetFileName(m_pFileName);
	patch.SetVerbose(m_bVerbose);
	patch.SetPatchFileName(m_pPatchFileName);
	return patch.ApplyPatchFile();
}

int C3DSTool::sample()
{
	printf("sample:\n");
	printf("# extract cci\n");
	printf("3dstool -xvt017f cci 0.cxi 1.cfa 7.cfa input.3ds --header ncsdheader.bin\n\n");
	printf("# extract cxi without encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin\n\n");
	printf("# extract cxi with AES-CTR encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# extract cxi with xor encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.Main.exheader.xorpad --exefs-xor 000400000XXXXX00.Main.exefs_norm.xorpad --romfs-xor 000400000XXXXX00.Main.romfs.xorpad\n\n");
	printf("# extract cxi with 7.x xor encryption\n");
	printf("3dstool -xvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.Main.exheader.xorpad --exefs-xor 000400000XXXXX00.Main.exefs_norm.xorpad --exefs-top-xor 000400000XXXXX00.Main.exefs_7x.xorpad --romfs-xor 000400000XXXXX00.Main.romfs.xorpad\n\n");
	printf("# extract cfa without encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin\n\n");
	printf("# extract cfa with AES-CTR encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# extract cfa with xor encryption\n");
	printf("3dstool -xvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.Manual.romfs.xorpad\n\n");
	printf("# extract exefs without Backward LZ77 uncompress\n");
	printf("3dstool -xvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# extract exefs with Backward LZ77 uncompress\n");
	printf("3dstool -xuvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# extract romfs\n");
	printf("3dstool -xvtf romfs romfs.bin --romfs-dir romfs\n\n");
	printf("# extract banner without LZ77Ex uncompress\n");
	printf("3dstool -xvtf banner banner.bnr --banner-dir banner\n\n");
	printf("# extract banner with LZ77Ex uncompress\n");
	printf("3dstool -xvtfu banner banner.bnr --banner-dir banner\n\n");
	printf("# create cci with pad 0xFF\n");
	printf("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin\n\n");
	printf("# create cci without pad\n");
	printf("3dstool -cvt017f cci 0.cxi 1.cfa 7.cfa output.3ds --header ncsdheader.bin --not-pad\n\n");
	printf("# create cxi without encryption but with calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin\n\n");
	printf("# create cxi without encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --not-update-exh-hash --not-update-exefs-hash --not-update-romfs-hash\n\n");
	printf("# create cxi with AES-CTR encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# create cxi with xor encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.Main.exheader.xorpad --exefs-xor 000400000XXXXX00.Main.exefs_norm.xorpad --romfs-xor 000400000XXXXX00.Main.romfs.xorpad\n\n");
	printf("# create cxi with 7.x xor encryption and calculate hash\n");
	printf("3dstool -cvtf cxi 0.cxi --header ncchheader.bin --exh exh.bin --logo logo.bcma.lz --plain plain.bin --exefs exefs.bin --romfs romfs.bin --exh-xor 000400000XXXXX00.Main.exheader.xorpad --exefs-xor 000400000XXXXX00.Main.exefs_norm.xorpad --exefs-top-xor 000400000XXXXX00.Main.exefs_7x.xorpad --romfs-xor 000400000XXXXX00.Main.romfs.xorpad\n\n");
	printf("# create cfa without encryption but with calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin\n\n");
	printf("# create cfa without encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --not-update-romfs-hash\n\n");
	printf("# create cfa with AES-CTR encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --key 00000000000000000000000000000000\n\n");
	printf("# create cfa with xor encryption and calculate hash\n");
	printf("3dstool -cvtf cfa 1.cfa --header ncchheader.bin --romfs romfs.bin --romfs-xor 000400000XXXXX00.Main.romfs.xorpad\n\n");
	printf("# create exefs without Backward LZ77 compress\n");
	printf("3dstool -cvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# create exefs with Backward LZ77 compress\n");
	printf("3dstool -czvtf exefs exefs.bin --header exefsheader.bin --exefs-dir exefs\n\n");
	printf("# create romfs without reference\n");
	printf("3dstool -cvtf romfs romfs.bin --romfs-dir romfs\n\n");
	printf("# create romfs with reference\n");
	printf("3dstool -cvtf romfs romfs.bin --romfs-dir romfs --romfs original_romfs.bin\n\n");
	printf("# create banner without LZ77Ex compress\n");
	printf("3dstool -cvtf banner banner.bnr --banner-dir banner\n\n");
	printf("# create banner with LZ77Ex compress\n");
	printf("3dstool -cvtfz banner banner.bnr --banner-dir banner\n\n");
	printf("# encrypt file with AES-CTR encryption, standalone\n");
	printf("3dstool -evf file.bin --key 00000000000000000000000000000000 --counter 00000000000000000000000000000000\n\n");
	printf("# encrypt file with xor encryption, standalone\n");
	printf("3dstool -evf file.bin --xor xor.bin\n\n");
	printf("# uncompress file with Backward LZ77, standalone\n");
	printf("3dstool -uvf code.bin --compress-type blz --compress-out code.bin\n\n");
	printf("# compress file with Backward LZ77, standalone\n");
	printf("3dstool -zvf code.bin --compress-type blz --compress-out code.bin\n\n");
	printf("# uncompress file with LZ77, standalone\n");
	printf("3dstool -uvf input.lz --compress-type lz --compress-out output.bin\n\n");
	printf("# compress file with LZ77, standalone\n");
	printf("3dstool -zvf output.lz --compress-type lz --compress-out input.bin\n\n");
	printf("# uncompress file with LZ77Ex, standalone\n");
	printf("3dstool -uvf logo.bcma.lz --compress-type lzex --compress-out logo.bcma\n\n");
	printf("# compress file with LZ77Ex, standalone\n");
	printf("3dstool -zvf logo.bcma.lz --compress-type lzex --compress-out logo.bcma\n\n");
	printf("# trim cci without pad\n");
	printf("3dstool -vtf cci input.3ds --trim\n\n");
	printf("# trim cci reserve partition 0~2\n");
	printf("3dstool -vtf cci input.3ds --trim --trim-after-partition 2\n\n");
	printf("# pad cci with 0xFF\n");
	printf("3dstool -vtf cci input.3ds --pad\n\n");
	printf("# create patch file without optimization\n");
	printf("3dstool --diff -v --old old.bin --new new.bin --patch-file patch.3ps\n\n");
	printf("# create patch file with cci optimization\n");
	printf("3dstool --diff -vt cci --old old.3ds --new new.3ds --patch-file patch.3ps\n\n");
	printf("# create patch file with cxi optimization\n");
	printf("3dstool --diff -vt cxi --old old.cxi --new new.cxi --patch-file patch.3ps\n\n");
	printf("# create patch file with cfa optimization\n");
	printf("3dstool --diff -vt cfa --old old.cfa --new new.cfa --patch-file patch.3ps\n\n");
	printf("# apply patch file\n");
	printf("3dstool --patch -vf input.bin --patch-file patch.3ps\n\n");
	return 0;
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
