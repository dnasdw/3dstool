#ifndef _3DSTOOL_H_
#define _3DSTOOL_H_

#include "utility.h"

class C3dsTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnOptionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExtract,
		kActionCreate,
		kActionEncrypt,
		kActionUncompress,
		kActionCompress,
		kActionTrim,
		kActionPad,
		kActionDiff,
		kActionPatch,
		kActionSample,
		kActionHelp
	};
	enum EFileType
	{
		kFileTypeUnknown,
		kFileTypeCci,
		kFileTypeCxi,
		kFileTypeCfa,
		kFileTypeExefs,
		kFileTypeRomfs,
		kFileTypeBanner
	};
	enum ECompressType
	{
		kCompressTypeNone,
		kCompressTypeBlz,
		kCompressTypeLz,
		kCompressTypeLzEx
	};
	struct SOption
	{
		const char* Name;
		int Key;
		const char* Doc;
	};
	C3dsTool();
	~C3dsTool();
	int ParseOptions(int a_nArgc, char* a_pArgv[]);
	int CheckOptions();
	int Help();
	int Action();
	static SOption s_Option[];
private:
	EParseOptionReturn parseOptions(const char* a_pName, int& a_nIndex, int a_nArgc, char* a_pArgv[]);
	EParseOptionReturn parseOptions(int a_nKey, int& a_nIndex, int a_nArgc, char* a_pArgv[]);
	bool checkFileType();
	bool extractFile();
	bool createFile();
	bool encryptFile();
	bool uncompressFile();
	bool compressFile();
	bool trimFile();
	bool padFile();
	bool diffFile();
	bool patchFile();
	int sample();
	EAction m_eAction;
	EFileType m_eFileType;
	const char* m_pFileName;
	bool m_bVerbose;
	const char* m_pHeaderFileName;
	int m_nEncryptMode;
	u8 m_uKey[16];
	string m_sCounter;
	const char* m_pXorFileName;
	n32 m_nCompressAlign;
	ECompressType m_eCompressType;
	const char* m_pCompressOutFileName;
	const char* m_pOldFileName;
	const char* m_pNewFileName;
	const char* m_pPatchFileName;
	const char* m_pNcchFileName[8];
	bool m_bNotPad;
	int m_nLastPartitionIndex;
	bool m_bNotUpdateExtendedHeaderHash;
	bool m_bNotUpdateExeFsHash;
	bool m_bNotUpdateRomFsHash;
	const char* m_pExtendedHeaderFileName;
	const char* m_pLogoRegionFileName;
	const char* m_pPlainRegionFileName;
	const char* m_pExeFsFileName;
	const char* m_pRomFsFileName;
	const char* m_pExtendedHeaderXorFileName;
	const char* m_pExeFsXorFileName;
	const char* m_pExeFsTopXorFileName;
	const char* m_pRomFsXorFileName;
	const char* m_pExeFsDirName;
	const char* m_pRomFsDirName;
	const char* m_pBannerDirName;
	bool m_bUncompress;
	bool m_bCompress;
	const char* m_pMessage;
};

#endif	// _3DSTOOL_H_
