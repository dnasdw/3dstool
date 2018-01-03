#ifndef EXTENDEDHEADER_H_
#define EXTENDEDHEADER_H_

#include "utility.h"

#include SDW_MSC_PUSH_PACKED
struct SystemInfoFlagStruct
{
	u8 reserved[5];
	u8 flag;
	u16 remasterVersion;
} SDW_GNUC_PACKED;

struct CodeSegmentInfo
{
	u32 Address;
	u32 NumMaxPages;
	u32 CodeSize;
} SDW_GNUC_PACKED;

struct CodeSetInfo
{
	u64 m_Name;
	SystemInfoFlagStruct m_Flags;
	CodeSegmentInfo m_TextSectionInfo;
	u32 m_StackSize;
	CodeSegmentInfo m_ReadOnlySectionInfo;
	u8 m_Reserved1[4];
	CodeSegmentInfo m_DataSectionInfo;
	u32 m_BssSize;
} SDW_GNUC_PACKED;

struct CoreInfo
{
	CodeSetInfo m_CodeSetInfo;
	u64 m_DepedencyList[48];
} SDW_GNUC_PACKED;

struct SystemInfoStruct
{
	u64 SaveDataSize;
	u64 JumpId;
	u8 Reserved2[48];
} SDW_GNUC_PACKED;

struct SystemControlInfo
{
	CoreInfo m_CoreInfo;
	SystemInfoStruct m_SystemInfo;
} SDW_GNUC_PACKED;

struct ARM11SystemLocalCapabilityFlags
{
	u32 CoreVersion;
	u8 Reserved[2];
	u8 IdealProcessor : 2;
	u8 AffinityMask : 2;
	u8 SystemMode : 4;
	u8 MainThreadPriority;
} SDW_GNUC_PACKED;

struct StorageInfoFlags
{
	u8 StorageAccessInfo[7];
	u8 OtherAttributes;
} SDW_GNUC_PACKED;

struct StorageInfoStruct
{
	u64 ExtSaveDataId;
	u64 SystemSaveDataId;
	u64 StorageAccessableUniqueIds;
	StorageInfoFlags InfoFlags;
} SDW_GNUC_PACKED;

struct ARM11SystemLocalCapabilities
{
	u64 m_ProgramId;
	ARM11SystemLocalCapabilityFlags m_Flags;
	u8 m_MaxCpu;
	u8 m_Reserved0;
	u8 m_ResourceLimits[15][2];
	StorageInfoStruct m_StorageInfo;
	u64 m_ServiceAccessControl[32];
	u8 m_Reserved[31];
	u8 m_ResourceLimitCategory;
} SDW_GNUC_PACKED;

struct ARM11KernelCapabilities
{
	u32 m_Descriptor[28];
	u8 m_Reserved[16];
} SDW_GNUC_PACKED;

struct AccessControlInfo
{
	ARM11SystemLocalCapabilities m_ARM11SystemLocalCapabilities;
	ARM11KernelCapabilities m_ARM11KernelCapabilities;
	u8 m_ARM9AccessControlInfo[16];
} SDW_GNUC_PACKED;

struct NcchExtendedHeader
{
	SystemControlInfo m_SystemControlInfo;
	AccessControlInfo m_AccessControlInfo;
} SDW_GNUC_PACKED;

struct NcchAccessControlExtended
{
	u8 m_RsaSignature[256];
	u8 m_NcchHeaderPublicKey[256];
	AccessControlInfo m_AccessControlInfoDescriptor;
} SDW_GNUC_PACKED;
#include SDW_MSC_POP_PACKED

#endif	// EXTENDEDHEADER_H_
