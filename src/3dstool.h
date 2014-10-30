#ifndef _3DS_TOOL_H_
#define _3DS_TOOL_H_

#include "utility.h"

class C3DSTool
{
public:
	enum EParseOptionReturn
	{
		kParseOptionReturnSuccess,
		kParseOptionReturnIllegalOption,
		kParseOptionReturnNoArgument,
		kParseOptionReturnUnknownArgument,
		kParseOptionReturnActionConflict
	};
	enum EAction
	{
		kActionNone,
		kActionExtract,
		kActionCreate,
		kActionCrypto,
		kActionHelp
	};
	enum EFileType
	{
		kFileTypeUnknown,
		kFileTypeCci,
		kFileTypeCxi,
		kFileTypeRomfs
	};
	struct SOption
	{
		const char* m_pName;
		int m_nKey;
		const char* m_pDoc;
	};
	C3DSTool();
	~C3DSTool();
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
	bool cryptoFile();
	EAction m_eAction;
	EFileType m_eFileType;
	const char* m_pFileName;
	const char* m_pHeaderFileName;
	const char* m_pNcchFileName[8];
	const char* m_pXorFileName;
	const char* m_pExtendedHeaderXorFileName;
	const char* m_pExeFsXorFileName;
	const char* m_pRomFsXorFileName;
	const char* m_pExtendedHeaderFileName;
	const char* m_pAccessControlExtendedFileName;
	const char* m_pPlainRegionFileName;
	const char* m_pExeFsFileName;
	const char* m_pRomFsFileName;
	const char* m_pRomFsDirName;
	bool m_bVerbose;
	const char* m_pMessage;
};

#endif	// _3DS_TOOL_H_
