#include "code.h"

CCode::CCode()
	: m_bVerbose(false)
	, m_nRegionCode(-1)
	, m_nLanguageCode(-1)
	, m_pArm(nullptr)
	, m_nArmCount(0)
	, m_pThumb(nullptr)
	, m_nThumbSize(0)
	, m_uHandle(0)
	, m_pInsn(nullptr)
	, m_uDisasmCount(0)
{
}

CCode::~CCode()
{
}

void CCode::SetFileName(const UString& a_sFileName)
{
	m_sFileName = a_sFileName;
}

void CCode::SetVerbose(bool a_bVerbose)
{
	m_bVerbose = a_bVerbose;
}

void CCode::SetRegionCode(n32 a_nRegionCode)
{
	m_nRegionCode = a_nRegionCode;
}

void CCode::SetLanguageCode(n32 a_nLanguageCode)
{
	m_nLanguageCode = a_nLanguageCode;
}

bool CCode::Lock()
{
	FILE* fpCode = UFopen(m_sFileName.c_str(), USTR("rb"));
	if (fpCode == nullptr)
	{
		return false;
	}
	Fseek(fpCode, 0, SEEK_END);
	u32 uCodeSize = static_cast<u32>(Ftell(fpCode));
	if (uCodeSize < 4)
	{
		fclose(fpCode);
		UPrintf(USTR("ERROR: code is too short\n\n"));
		return false;
	}
	m_vCode.resize(uCodeSize);
	Fseek(fpCode, 0, SEEK_SET);
	fread(&*m_vCode.begin(), 1, uCodeSize, fpCode);
	fclose(fpCode);
	m_pArm = reinterpret_cast<u32*>(&*m_vCode.begin());
	m_nArmCount = uCodeSize / 4;
	m_pThumb = &*m_vCode.begin();
	m_nThumbSize = uCodeSize / 4 * 4;
	bool bResultArm = lockArm();
	bool bResultThumb = false;
	if (!bResultArm)
	{
		bResultThumb = lockThumb();
	}
	fpCode = UFopen(m_sFileName.c_str(), USTR("wb"));
	if (fpCode == nullptr)
	{
		return false;
	}
	fwrite(&*m_vCode.begin(), 1, uCodeSize, fpCode);
	fclose(fpCode);
	if (!bResultArm && !bResultThumb)
	{
		UPrintf(USTR("ERROR: lock failed\n\n"));
	}
	return bResultArm || bResultThumb;
}

bool CCode::lockArm()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: lock arm\n"));
	}
	bool bResult = true;
	if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &m_uHandle) != CS_ERR_OK)
	{
		UPrintf(USTR("ERROR: open arm handle failed\n\n"));
		return false;
	}
	cs_option(m_uHandle, CS_OPT_DETAIL, CS_OPT_ON);
	if (m_nRegionCode >= 0)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: lock region arm\n"));
		}
		bool bResultRegion = lockRegionArm();
		if (!bResultRegion)
		{
			UPrintf(USTR("ERROR: lock region arm failed\n\n"));
		}
		bResult = bResult && bResultRegion;
	}
	if (m_nLanguageCode >= 0)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: lock language arm\n"));
		}
		bool bResultLanguage = lockLanguageArm();
		if (!bResultLanguage)
		{
			UPrintf(USTR("ERROR: lock language arm failed\n\n"));
		}
		bResult = bResult && bResultLanguage;
	}
	cs_close(&m_uHandle);
	if (!bResult)
	{
		UPrintf(USTR("ERROR: lock arm failed\n\n"));
	}
	return bResult;
}

bool CCode::lockThumb()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: lock thumb\n"));
	}
	bool bResult = true;
	if (cs_open(CS_ARCH_ARM, CS_MODE_THUMB, &m_uHandle) != CS_ERR_OK)
	{
		UPrintf(USTR("ERROR: open thumb handle failed\n\n"));
		return false;
	}
	cs_option(m_uHandle, CS_OPT_DETAIL, CS_OPT_ON);
	if (m_nRegionCode >= 0)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: lock region thumb\n"));
		}
		bool bResultRegion = lockRegionThumb();
		if (!bResultRegion)
		{
			UPrintf(USTR("ERROR: lock region thumb failed\n\n"));
		}
		bResult = bResult && bResultRegion;
	}
	if (m_nLanguageCode >= 0)
	{
		if (m_bVerbose)
		{
			UPrintf(USTR("INFO: lock language thumb\n"));
		}
		bool bResultLanguage = lockLanguageThumb();
		if (!bResultLanguage)
		{
			UPrintf(USTR("ERROR: lock language thumb failed\n\n"));
		}
		bResult = bResult && bResultLanguage;
	}
	cs_close(&m_uHandle);
	if (!bResult)
	{
		UPrintf(USTR("ERROR: lock thumb failed\n\n"));
	}
	return bResult;
}

bool CCode::lockRegionArm()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: find arm nn::cfg::CTR::detail::IpcUser::GetRegion\n"));
	}
	SFunction functionGetRegion = {};
	findGetRegionFunctionArm(functionGetRegion);
	if (functionGetRegion.First == functionGetRegion.Last || functionGetRegion.Last == 0)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: nn::cfg::CTR::detail::IpcUser::GetRegion\n"));
		UPrintf(USTR("INFO:   function:\n"));
		UPrintf(USTR("INFO:     first: %08X\n"), functionGetRegion.First * 4);
		UPrintf(USTR("INFO:     last:  %08X\n"), functionGetRegion.Last * 4);
	}
	return patchGetRegionFunctionArm(functionGetRegion);
}

bool CCode::lockRegionThumb()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: find thumb nn::cfg::CTR::detail::IpcUser::GetRegion\n"));
	}
	SFunction functionGetRegion = {};
	findGetRegionFunctionThumb(functionGetRegion);
	if (functionGetRegion.First == functionGetRegion.Last || functionGetRegion.Last == 0)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: nn::cfg::CTR::detail::IpcUser::GetRegion\n"));
		UPrintf(USTR("INFO:   function:\n"));
		UPrintf(USTR("INFO:     first: %08X\n"), functionGetRegion.First);
		UPrintf(USTR("INFO:     last:  %08X\n"), functionGetRegion.Last);
	}
	return patchGetRegionFunctionThumb(functionGetRegion);
}

bool CCode::lockLanguageArm()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: find arm nn::cfg::CTR::GetLanguage\n"));
	}
	SFunction functionGetLanguage = {};
	findGetLanguageFunctionArm(functionGetLanguage);
	if (functionGetLanguage.First == functionGetLanguage.Last || functionGetLanguage.Last == 0)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: nn::cfg::CTR::GetLanguage\n"));
		UPrintf(USTR("INFO:   function:\n"));
		UPrintf(USTR("INFO:     first: %08X\n"), functionGetLanguage.First * 4);
		UPrintf(USTR("INFO:     last:  %08X\n"), functionGetLanguage.Last * 4);
	}
	return patchGetLanguageFunctionArm(functionGetLanguage);
}

bool CCode::lockLanguageThumb()
{
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: find thumb nn::cfg::CTR::GetLanguageRaw\n"));
	}
	SFunction functionGetLanguage = {};
	findGetLanguageFunctionThumb(functionGetLanguage);
	if (functionGetLanguage.First == functionGetLanguage.Last || functionGetLanguage.Last == 0)
	{
		return false;
	}
	if (m_bVerbose)
	{
		UPrintf(USTR("INFO: nn::cfg::CTR::GetLanguageRaw\n"));
		UPrintf(USTR("INFO:   function:\n"));
		UPrintf(USTR("INFO:     first: %08X\n"), functionGetLanguage.First);
		UPrintf(USTR("INFO:     last:  %08X\n"), functionGetLanguage.Last);
	}
	return patchGetLanguageFunctionThumb(functionGetLanguage);
}

// nn::cfg::CTR::detail::IpcUser::GetRegion
void CCode::findGetRegionFunctionArm(SFunction& a_Function)
{
	vector<SFunction> vFunction;
	for (n32 i = 0; i < m_nArmCount; i++)
	{
		// mov r0, #0x20000
		if (m_pArm[i] == 0xE3A00802)
		{
			SFunction function = { i, i };
			findFunctionArm(function);
			for (n32 j = i + 1; j < function.Last; j++)
			{
				// svc 0x32
				if (m_pArm[j] == 0xEF000032)
				{
					vFunction.push_back(function);
					break;
				}
			}
		}
	}
	// nn::cfg::CTR::detail::Initialize
	for (n32 i = 0; i < m_nArmCount; i++)
	{
		// nn::srv::Initialize
		// nn::Result
		// Level	-5 LEVEL_PERMANENT
		// Summary	5 SUMMARY_INVALID_STATE
		// Module	64 MODULE_NN_CFG
		if (m_pArm[i] == 0xD8A103F9)
		{
			for (n32 j = i - 4; j < i + 4; j++)
			{
				if (j >= 0 && j < m_nArmCount)
				{
					for (vector<SFunction>::iterator it = vFunction.begin(); it != vFunction.end(); ++it)
					{
						SFunction& function = *it;
						// nn::cfg::CTR::detail::IpcUser::s_Session
						if (function.Last + 1 < m_nArmCount && m_pArm[j] == m_pArm[function.Last + 1])
						{
							a_Function.First = function.First;
							a_Function.Last = function.Last;
							return;
						}
					}
				}
			}
		}
	}
	for (n32 i = 0; i < m_nArmCount; i++)
	{
		// mov r0, #0x20000
		if (m_pArm[i] == 0xE3A00802)
		{
			SFunction function = { i, i };
			findFunctionArm(function);
			for (n32 j = i + 1; j < function.Last; j++)
			{
				// nn::svc::SendSyncRequest
				m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + j), 4, 0x100000 + j * 4, 0, &m_pInsn);
				if (m_uDisasmCount > 0)
				{
					if (strcmp(m_pInsn->mnemonic, "bl") == 0 && m_pInsn->detail != nullptr)
					{
						cs_arm* pDetail = &m_pInsn->detail->arm;
						if (pDetail->op_count == 1)
						{
							cs_arm_op* pArmOp0 = &pDetail->operands[0];
							if (pArmOp0->type == ARM_OP_IMM && pArmOp0->imm >= 0x100000 && pArmOp0->imm + 8 <= 0x100000 + m_nArmCount * 4 && pArmOp0->imm % 4 == 0)
							{
								n32 nFunction = (pArmOp0->imm - 0x100000) / 4;
								// svc 0x32
								// bx lr
								if (m_pArm[nFunction] == 0xEF000032 && m_pArm[nFunction + 1] == 0xE12FFF1E)
								{
									vFunction.push_back(function);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	// nn::cfg::CTR::detail::Initialize
	for (n32 i = 0; i < m_nArmCount; i++)
	{
		// nn::srv::Initialize
		// nn::Result
		// Level	-5 LEVEL_PERMANENT
		// Summary	5 SUMMARY_INVALID_STATE
		// Module	64 MODULE_NN_CFG
		if (m_pArm[i] == 0xD8A103F9)
		{
			for (n32 j = i - 4; j < i + 4; j++)
			{
				if (j >= 0 && j < m_nArmCount)
				{
					for (vector<SFunction>::iterator it = vFunction.begin(); it != vFunction.end(); ++it)
					{
						SFunction& function = *it;
						// nn::cfg::CTR::detail::IpcUser::s_Session
						if (function.Last + 1 < m_nArmCount && m_pArm[j] == m_pArm[function.Last + 1])
						{
							a_Function.First = function.First;
							a_Function.Last = function.Last;
							return;
						}
					}
				}
			}
		}
	}
}

// nn::cfg::CTR::detail::IpcUser::GetRegion
void CCode::findGetRegionFunctionThumb(SFunction& a_Function)
{
	SFunction functionGetLanguage;
	findGetLanguageFunctionThumb(functionGetLanguage);
	if (functionGetLanguage.First == functionGetLanguage.Last || functionGetLanguage.Last == 0)
	{
		return;
	}
	// nn::cfg::CTR::GetLanguage
	// nn::cfg::CTR::GetLanguageRaw()
	n32 nGetLanguage = -1;
	n32 nCodeSizeMax = 4;
	for (n32 i = 0; i < m_nThumbSize; i += 2)
	{
		bool bOver = false;
		nCodeSizeMax = m_nThumbSize - i;
		if (nCodeSizeMax > 4)
		{
			nCodeSizeMax = 4;
		}
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if (m_uDisasmCount > 0 && !cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			if (strcmp(m_pInsn->mnemonic, "bl") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count == 1)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_IMM && pArmOp0->imm == 0x100000 + functionGetLanguage.First - 2)
					{
						bOver = true;
					}
				}
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			nGetLanguage = i;
			break;
		}
	}
	if (nGetLanguage < 0)
	{
		return;
	}
	n32 nGetRegion = -1;
	n32 nCodeSize = 4;
	for (n32 i = nGetLanguage + 4; i < m_nThumbSize; i += nCodeSize)
	{
		bool bOver = false;
		n32 nCodeSizeMax = m_nThumbSize - i;
		if (nCodeSizeMax > 4)
		{
			nCodeSizeMax = 4;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if (m_uDisasmCount > 0 && !cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			nCodeSize = m_pInsn->size;
			if (strcmp(m_pInsn->mnemonic, "bl") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count == 1)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_IMM && pArmOp0->imm >= 0x100000 && pArmOp0->imm < 0x100000 + m_nThumbSize)
					{
						nGetRegion = pArmOp0->imm - 0x100000;
						bOver = true;
					}
				}
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			break;
		}
	}
	SFunction functionGetRegion = { nGetRegion, nGetRegion };
	findFunctionThumb(functionGetRegion);
	for (n32 i = functionGetRegion.First + 2; i < functionGetRegion.Last; i += nCodeSize)
	{
		bool bOver = false;
		n32 nCodeSizeMax = m_nThumbSize - i;
		if (nCodeSizeMax > 4)
		{
			nCodeSizeMax = 4;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if (m_uDisasmCount > 0 && !cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			nCodeSize = m_pInsn->size;
			if (strcmp(m_pInsn->mnemonic, "bl") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count == 1)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_IMM && pArmOp0->imm >= 0x100000 && pArmOp0->imm < 0x100000 + m_nThumbSize)
					{
						nGetRegion = pArmOp0->imm - 0x100000;
						bOver = true;
					}
				}
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			break;
		}
	}
	a_Function.First = nGetRegion + 4;
	a_Function.Last = a_Function.First;
	findFunctionThumb(a_Function);
}

// nn::cfg::CTR::GetLanguage
void CCode::findGetLanguageFunctionArm(SFunction& a_Function)
{
	for (n32 i = 0; i < m_nArmCount; i++)
	{
		// nn::cfg::CTR::detail::GetConfig
		// key
		if (m_pArm[i] == 0xA0002)
		{
			n32 nIndex = i - 4;
			if (nIndex < 0)
			{
				nIndex = 0;
			}
			SFunction function = { nIndex, nIndex };
			findFunctionArm(function);
			for (n32 j = function.First + 1; j < function.Last; j++)
			{
				bool bOver = false;
				m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + j), 4, 0x100000 + j * 4, 0, &m_pInsn);
				if (m_uDisasmCount > 0)
				{
					if (strcmp(m_pInsn->mnemonic, "ldr") == 0 && m_pInsn->detail != nullptr)
					{
						cs_arm* pDetail = &m_pInsn->detail->arm;
						if (pDetail->op_count > 1)
						{
							cs_arm_op* pArmOp0 = &pDetail->operands[0];
							cs_arm_op* pArmOp1 = &pDetail->operands[1];
							// ldr rm, =0xA0002
							if (pArmOp0->type == ARM_OP_REG && pArmOp1->type == ARM_OP_MEM && pArmOp1->reg == ARM_REG_PC && pArmOp1->mem.disp == (i - j - 2) * 4)
							{
								bOver = true;
							}
						}
					}
				}
				cs_free(m_pInsn, m_uDisasmCount);
				if (bOver)
				{
					a_Function.First = function.First;
					a_Function.Last = function.Last;
					return;
				}
			}
		}
	}
}

// nn::cfg::CTR::GetLanguageRaw
void CCode::findGetLanguageFunctionThumb(SFunction& a_Function)
{
	for (n32 i = 0; i < m_nThumbSize; i += 4)
	{
		// nn::cfg::CTR::detail::GetConfig
		// key
		if (*reinterpret_cast<u32*>(m_pThumb + i) == 0xA0002)
		{
			n32 nOffset = i - 16;
			if (nOffset < 0)
			{
				nOffset = 0;
			}
			SFunction function = { nOffset, nOffset };
			findFunctionThumb(function);
			n32 nCodeSize = 4;
			for (n32 j = function.First; j < function.Last; j += nCodeSize)
			{
				bool bOver = false;
				n32 nCodeSizeMax = function.Last - j;
				if (nCodeSizeMax > 4)
				{
					nCodeSizeMax = 4;
				}
				nCodeSize = 2;
				m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + j, nCodeSizeMax, 0x100000 + j, 0, &m_pInsn);
				if (m_uDisasmCount > 0 && !cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
				{
					nCodeSize = m_pInsn->size;
					if (strcmp(m_pInsn->mnemonic, "ldr") == 0 && m_pInsn->detail != nullptr)
					{
						cs_arm* pDetail = &m_pInsn->detail->arm;
						if (pDetail->op_count > 1)
						{
							cs_arm_op* pArmOp0 = &pDetail->operands[0];
							cs_arm_op* pArmOp1 = &pDetail->operands[1];
							// ldr rm, =0xA0002
							if (pArmOp0->type == ARM_OP_REG && pArmOp1->type == ARM_OP_MEM && pArmOp1->reg == ARM_REG_PC && pArmOp1->mem.disp == i - j - 4)
							{
								bOver = true;
							}
						}
					}
				}
				cs_free(m_pInsn, m_uDisasmCount);
				if (bOver)
				{
					a_Function.First = function.First;
					a_Function.Last = function.Last;
					return;
				}
			}
		}
	}
}

void CCode::findFunctionArm(SFunction& a_Function)
{
	for (n32 i = a_Function.Last; i < m_nArmCount; i++)
	{
		bool bOver = false;
		m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + i), 4, 0x100000 + i * 4, 0, &m_pInsn);
		if (m_uDisasmCount > 0)
		{
			if (strcmp(m_pInsn->mnemonic, "pop") == 0 || (strcmp(m_pInsn->mnemonic, "bx") == 0 && strcmp(m_pInsn->op_str, "lr") == 0))
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			a_Function.Last = i;
			break;
		}
	}
	for (n32 i = a_Function.First; i >= 0; i--)
	{
		bool bOver = false;
		m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + i), 4, 0x100000 + i * 4, 0, &m_pInsn);
		if (m_uDisasmCount > 0)
		{
			if (strcmp(m_pInsn->mnemonic, "push") == 0)
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			a_Function.First = i;
			break;
		}
	}
}

void CCode::findFunctionThumb(SFunction& a_Function)
{
	n32 nCodeSize = 4;
	n32 nCodeSizeMax = 4;
	for (n32 i = a_Function.Last; i < m_nThumbSize; i += nCodeSize)
	{
		bool bOver = false;
		nCodeSizeMax = m_nThumbSize - i;
		if (nCodeSizeMax > 4)
		{
			nCodeSizeMax = 4;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if (m_uDisasmCount > 0 && !cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			nCodeSize = m_pInsn->size;
			if (strcmp(m_pInsn->mnemonic, "pop") == 0 || (strcmp(m_pInsn->mnemonic, "bx") == 0 && strcmp(m_pInsn->op_str, "lr") == 0))
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			a_Function.Last = i;
			break;
		}
	}
	n32 nCodeSizeCache = 0;
	for (n32 i = a_Function.First; i >= 0; i -= nCodeSize)
	{
		bool bOver = false;
		if (nCodeSizeCache == 0)
		{
			nCodeSizeMax = m_nThumbSize - i;
			if (nCodeSizeMax > 4)
			{
				nCodeSizeMax = 4;
			}
		}
		else
		{
			nCodeSizeMax = nCodeSizeCache;
			nCodeSizeCache = 0;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if ((m_uDisasmCount == 0 && i >= 2) || cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			i -= 2;
			nCodeSizeMax += 2;
			m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		}
		if (m_uDisasmCount == 2)
		{
			nCodeSizeCache = m_pInsn->size;
			i += nCodeSizeCache;
			nCodeSizeMax -= nCodeSizeCache;
			m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		}
		if (m_uDisasmCount > 0)
		{
			if (nCodeSizeCache == 0)
			{
				nCodeSize = 4;
				if (i < 4)
				{
					nCodeSize = 2;
				}
			}
			else
			{
				nCodeSize = nCodeSizeCache;
			}
			if (strcmp(m_pInsn->mnemonic, "push") == 0)
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			a_Function.First = i;
			break;
		}
	}
}

bool CCode::patchGetRegionFunctionArm(SFunction& a_Function)
{
	for (n32 i = a_Function.First + 1; i < a_Function.Last; i++)
	{
		bool bOver = false;
		n32 nRt = -1;
		m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + i), 4, 0x100000 + i * 4, 0, &m_pInsn);
		if (m_uDisasmCount > 0)
		{
			if (strcmp(m_pInsn->mnemonic, "ldrb") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count > 0)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_REG)
					{
						switch (pArmOp0->reg)
						{
						case ARM_REG_R0:
							nRt = 0;
							break;
						case ARM_REG_R1:
							nRt = 1;
							break;
						case ARM_REG_R2:
							nRt = 2;
							break;
						case ARM_REG_R3:
							nRt = 3;
							break;
						case ARM_REG_R4:
							nRt = 4;
							break;
						case ARM_REG_R5:
							nRt = 5;
							break;
						case ARM_REG_R6:
							nRt = 6;
							break;
						case ARM_REG_R7:
							nRt = 7;
							break;
						case ARM_REG_R8:
							nRt = 8;
							break;
						case ARM_REG_R9:
							nRt = 9;
							break;
						case ARM_REG_R10:
							nRt = 10;
							break;
						case ARM_REG_R11:
							nRt = 11;
							break;
						case ARM_REG_R12:
							nRt = 12;
							break;
						case ARM_REG_R13:
							// ARM_REG_SP
							nRt = 13;
							break;
						case ARM_REG_R14:
							// ARM_REG_LR
							nRt = 14;
							break;
						case ARM_REG_R15:
							// ARM_REG_PC
							nRt = 15;
							break;
						}
					}
				}
			}
			if (nRt >= 0)
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			m_pArm[i] = 0xE3A00000 | (nRt << 12) | m_nRegionCode;
			UPrintf(USTR("INFO:   modify:  %08X  mov r%d, #0x%x ; %02X %02X %02X %02X\n"), i * 4, nRt, m_nRegionCode, m_pArm[i] & 0xFF, m_pArm[i] >> 8 & 0xFF, m_pArm[i] >> 16 & 0xFF, m_pArm[i] >> 24 & 0xFF);
			return true;
		}
	}
	return false;
}

bool CCode::patchGetRegionFunctionThumb(SFunction& a_Function)
{
	n32 nCodeSize = 4;
	for (n32 i = a_Function.First + 2; i < a_Function.Last; i += nCodeSize)
	{
		bool bOver = false;
		n32 nRt = -1;
		n32 nCodeSizeMax = m_nThumbSize - i;
		if (nCodeSizeMax > 4)
		{
			nCodeSizeMax = 4;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if (m_uDisasmCount > 0)
		{
			nCodeSize = m_pInsn->size;
			if (strcmp(m_pInsn->mnemonic, "ldrb") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count > 0)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_REG)
					{
						switch (pArmOp0->reg)
						{
						case ARM_REG_R0:
							nRt = 0;
							break;
						case ARM_REG_R1:
							nRt = 1;
							break;
						case ARM_REG_R2:
							nRt = 2;
							break;
						case ARM_REG_R3:
							nRt = 3;
							break;
						case ARM_REG_R4:
							nRt = 4;
							break;
						case ARM_REG_R5:
							nRt = 5;
							break;
						case ARM_REG_R6:
							nRt = 6;
							break;
						case ARM_REG_R7:
							nRt = 7;
							break;
						case ARM_REG_R8:
							nRt = 8;
							break;
						case ARM_REG_R9:
							nRt = 9;
							break;
						case ARM_REG_R10:
							nRt = 10;
							break;
						case ARM_REG_R11:
							nRt = 11;
							break;
						case ARM_REG_R12:
							nRt = 12;
							break;
						case ARM_REG_R13:
							// ARM_REG_SP
							nRt = 13;
							break;
						case ARM_REG_R14:
							// ARM_REG_LR
							nRt = 14;
							break;
						case ARM_REG_R15:
							// ARM_REG_PC
							nRt = 15;
							break;
						}
					}
				}
			}
			if (nRt >= 0)
			{
				bOver = true;
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			*reinterpret_cast<u16*>(m_pThumb + i) = 0x2000 | (nRt << 8) | m_nRegionCode;
			UPrintf(USTR("INFO:   modify:  %08X  mov r%d, #0x%x ; %02X %02X\n"), i, nRt, m_nRegionCode, m_pThumb[i], m_pThumb[i + 1]);
			return true;
		}
	}
	return false;
}

bool CCode::patchGetLanguageFunctionArm(SFunction& a_Function)
{
	for (n32 i = a_Function.Last - 1; i > a_Function.First; i--)
	{
		bool bOver = false;
		m_uDisasmCount = cs_disasm(m_uHandle, reinterpret_cast<u8*>(m_pArm + i), 4, 0x100000 + i * 4, 0, &m_pInsn);
		if (m_uDisasmCount > 0)
		{
			if (strcmp(m_pInsn->mnemonic, "ldrb") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count > 0)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_REG && pArmOp0->reg == ARM_REG_R0)
					{
						bOver = true;
					}
				}
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			m_pArm[i] = 0xE3A00000 | m_nLanguageCode;
			UPrintf(USTR("INFO:   modify:  %08X  mov r0, #0x%x ; %02X %02X %02X %02X\n"), i * 4, m_nLanguageCode, m_pArm[i] & 0xFF, m_pArm[i] >> 8 & 0xFF, m_pArm[i] >> 16 & 0xFF, m_pArm[i] >> 24 & 0xFF);
			return true;
		}
	}
	return false;
}

bool CCode::patchGetLanguageFunctionThumb(SFunction& a_Function)
{
	n32 nCodeSize = 4;
	n32 nCodeSizeMax = 4;
	n32 nCodeSizeCache = 0;
	for (n32 i = a_Function.Last - 2; i > a_Function.First; i -= nCodeSize)
	{
		bool bOver = false;
		if (nCodeSizeCache == 0)
		{
			nCodeSizeMax = m_nThumbSize - i;
			if (nCodeSizeMax > 4)
			{
				nCodeSizeMax = 4;
			}
		}
		else
		{
			nCodeSizeMax = nCodeSizeCache;
			nCodeSizeCache = 0;
		}
		nCodeSize = 2;
		m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		if ((m_uDisasmCount == 0 && i > a_Function.First + 2) || cs_insn_group(m_uHandle, m_pInsn, ARM_GRP_THUMB2))
		{
			i -= 2;
			nCodeSizeMax += 2;
			m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		}
		if (m_uDisasmCount == 2)
		{
			nCodeSizeCache = m_pInsn->size;
			i += nCodeSizeCache;
			nCodeSizeMax -= nCodeSizeCache;
			m_uDisasmCount = cs_disasm(m_uHandle, m_pThumb + i, nCodeSizeMax, 0x100000 + i, 0, &m_pInsn);
		}
		if (m_uDisasmCount > 0)
		{
			if (nCodeSizeCache == 0)
			{
				nCodeSize = 4;
				if (i <= a_Function.Last + 4)
				{
					nCodeSize = 2;
				}
			}
			else
			{
				nCodeSize = nCodeSizeCache;
			}
			if (strcmp(m_pInsn->mnemonic, "ldrb") == 0 && m_pInsn->detail != nullptr)
			{
				cs_arm* pDetail = &m_pInsn->detail->arm;
				if (pDetail->op_count > 0)
				{
					cs_arm_op* pArmOp0 = &pDetail->operands[0];
					if (pArmOp0->type == ARM_OP_REG && pArmOp0->reg == ARM_REG_R0)
					{
						bOver = true;
					}
				}
			}
		}
		cs_free(m_pInsn, m_uDisasmCount);
		if (bOver)
		{
			*reinterpret_cast<u16*>(m_pThumb + i) = 0x2000 | m_nLanguageCode;
			UPrintf(USTR("INFO:   modify:  %08X  mov r0, #0x%x ; %02X %02X\n"), i, m_nLanguageCode, m_pThumb[i], m_pThumb[i + 1]);
			return true;
		}
	}
	return false;
}
