/*
 * Copyright (c) 2011 The Khronos Group Inc. 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions: 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 *
 */

/*
 *  OMX_Component.h - OpenMax IL version 1.2.0
 *  The OMX_Component header file contains the definitions used
 *  to define the public interface of a component.  This header
 *  file is intended to be used by both the application and the
 *  component.
 */

#ifndef OMX_Component_h
#define OMX_Component_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header must include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully 
 */

#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_Image.h>
#include <OMX_Other.h>

typedef enum OMX_PORTDOMAINTYPE { 
    OMX_PortDomainAudio, 
    OMX_PortDomainVideo, 
    OMX_PortDomainImage, 
    OMX_PortDomainOther,
    OMX_PortDomainKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_PortDomainVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_PortDomainMax = 0x7ffffff
} OMX_PORTDOMAINTYPE;

typedef struct OMX_PARAM_PORTDEFINITIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_DIRTYPE eDir;
    OMX_U32 nBufferCountActual;
    OMX_U32 nBufferCountMin;
    OMX_U32 nBufferSize;
    OMX_BOOL bEnabled;
    OMX_BOOL bPopulated;
    OMX_PORTDOMAINTYPE eDomain;
    union {
        OMX_AUDIO_PORTDEFINITIONTYPE audio;
        OMX_VIDEO_PORTDEFINITIONTYPE video;
        OMX_IMAGE_PORTDEFINITIONTYPE image;
        OMX_OTHER_PORTDEFINITIONTYPE other;
    } format;
    OMX_BOOL bBuffersContiguous;
    OMX_U32 nBufferAlignment;
} OMX_PARAM_PORTDEFINITIONTYPE;

typedef struct OMX_PARAM_U32TYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nU32;
} OMX_PARAM_U32TYPE;

typedef enum OMX_SUSPENSIONPOLICYTYPE {
    OMX_SuspensionDisabled,
    OMX_SuspensionEnabled,
    OMX_SuspensionPolicyKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_SuspensionPolicyStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_SuspensionPolicyMax = 0x7fffffff
} OMX_SUSPENSIONPOLICYTYPE;

typedef struct OMX_PARAM_SUSPENSIONPOLICYTYPE {
    OMX_U32 nSize;                  
    OMX_VERSIONTYPE nVersion;        
    OMX_SUSPENSIONPOLICYTYPE ePolicy;
} OMX_PARAM_SUSPENSIONPOLICYTYPE;

typedef enum OMX_SUSPENSIONTYPE {
    OMX_NotSuspended,
    OMX_Suspended,
    OMX_SuspensionKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_SuspensionVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_SuspendMax = 0x7FFFFFFF
} OMX_SUSPENSIONTYPE;

typedef struct OMX_PARAM_SUSPENSIONTYPE {
    OMX_U32 nSize;                  
    OMX_VERSIONTYPE nVersion;       
    OMX_SUSPENSIONTYPE eType;             
} OMX_PARAM_SUSPENSIONTYPE ;

typedef struct OMX_CONFIG_BOOLEANTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bEnabled;    
} OMX_CONFIG_BOOLEANTYPE;

typedef struct OMX_PARAM_CONTENTURITYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U8 contentURI[1];
} OMX_PARAM_CONTENTURITYPE;

typedef struct OMX_RESOURCECONCEALMENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bResourceConcealmentForbidden;
} OMX_RESOURCECONCEALMENTTYPE;

typedef enum OMX_METADATACHARSETTYPE {
    OMX_MetadataCharsetUnknown = 0,
    OMX_MetadataCharsetASCII,
    OMX_MetadataCharsetBinary,
    OMX_MetadataCharsetCodePage1252,
    OMX_MetadataCharsetUTF8,
    OMX_MetadataCharsetJavaConformantUTF8,
    OMX_MetadataCharsetUTF7,
    OMX_MetadataCharsetImapUTF7,
    OMX_MetadataCharsetUTF16LE, 
    OMX_MetadataCharsetUTF16BE,
    OMX_MetadataCharsetGB12345,
    OMX_MetadataCharsetHZGB2312,
    OMX_MetadataCharsetGB2312,
    OMX_MetadataCharsetGB18030,
    OMX_MetadataCharsetGBK,
    OMX_MetadataCharsetBig5,
    OMX_MetadataCharsetISO88591,
    OMX_MetadataCharsetISO88592,
    OMX_MetadataCharsetISO88593,
    OMX_MetadataCharsetISO88594,
    OMX_MetadataCharsetISO88595,
    OMX_MetadataCharsetISO88596,
    OMX_MetadataCharsetISO88597,
    OMX_MetadataCharsetISO88598,
    OMX_MetadataCharsetISO88599,
    OMX_MetadataCharsetISO885910,
    OMX_MetadataCharsetISO885913,
    OMX_MetadataCharsetISO885914,
    OMX_MetadataCharsetISO885915,
    OMX_MetadataCharsetShiftJIS,
    OMX_MetadataCharsetISO2022JP,
    OMX_MetadataCharsetISO2022JP1,
    OMX_MetadataCharsetISOEUCJP,
    OMX_MetadataCharsetSMS7Bit,
    OMX_MetadataCharsetKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_MetadataCharsetVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_MetadataCharsetTypeMax= 0x7FFFFFFF
} OMX_METADATACHARSETTYPE;

typedef enum OMX_METADATASCOPETYPE
{
    OMX_MetadataScopeAllLevels,
    OMX_MetadataScopeTopLevel,
    OMX_MetadataScopePortLevel,
    OMX_MetadataScopeNodeLevel,
    OMX_MetadataScopeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_MetadataScopeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_MetadataScopeTypeMax = 0x7fffffff
} OMX_METADATASCOPETYPE;

typedef enum OMX_METADATASEARCHMODETYPE
{
    OMX_MetadataSearchValueSizeByIndex,
    OMX_MetadataSearchItemByIndex,
    OMX_MetadataSearchNextItemByKey,
    OMX_MetadataSearchKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_MetadataSearchVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_MetadataSearchTypeMax = 0x7fffffff
} OMX_METADATASEARCHMODETYPE;

typedef struct OMX_CONFIG_METADATAITEMCOUNTTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_METADATASCOPETYPE eScopeMode;
    OMX_U32 nScopeSpecifier;
    OMX_U32 nMetadataItemCount;
} OMX_CONFIG_METADATAITEMCOUNTTYPE;

typedef struct OMX_CONFIG_METADATAITEMTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_METADATASCOPETYPE eScopeMode;
    OMX_U32 nScopeSpecifier;
    OMX_U32 nMetadataItemIndex;  
    OMX_METADATASEARCHMODETYPE eSearchMode;
    OMX_METADATACHARSETTYPE eKeyCharset;
    OMX_U8 nKeySizeUsed;
    OMX_U8 nKey[128];
    OMX_METADATACHARSETTYPE eValueCharset;
    OMX_U8 sLanguageCountry[128];
    OMX_U32 nValueMaxSize;
    OMX_U32 nValueSizeUsed;
    OMX_U8 nValue[1];
} OMX_CONFIG_METADATAITEMTYPE;

typedef struct OMX_CONFIG_CONTAINERNODECOUNTTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bAllKeys;
    OMX_U32 nParentNodeID;
    OMX_U32 nNumNodes;
} OMX_CONFIG_CONTAINERNODECOUNTTYPE;

typedef struct OMX_CONFIG_CONTAINERNODEIDTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bAllKeys;
    OMX_U32 nParentNodeID;
    OMX_U32 nNodeIndex; 
    OMX_U32 nNodeID; 
    OMX_U8 cNodeName[128];
    OMX_BOOL bIsLeafType;
} OMX_CONFIG_CONTAINERNODEIDTYPE;

typedef struct OMX_PARAM_METADATAFILTERTYPE 
{ 
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_BOOL bAllKeys;
    OMX_METADATACHARSETTYPE eKeyCharset;
    OMX_U32 nKeySizeUsed; 
    OMX_U8 nKey [128]; 
    OMX_U32 nLanguageCountrySizeUsed;
    OMX_U8 nLanguageCountry[128];
    OMX_BOOL bEnabled;
} OMX_PARAM_METADATAFILTERTYPE; 

typedef struct OMX_COMPONENTTYPE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_PTR pComponentPrivate;
    OMX_PTR pApplicationPrivate;

    OMX_ERRORTYPE (*GetComponentVersion)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STRING pComponentName,
            OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
            OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
            OMX_OUT OMX_UUIDTYPE* pComponentUUID);

    OMX_ERRORTYPE (*SendCommand)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_COMMANDTYPE Cmd,
            OMX_IN  OMX_U32 nParam1,
            OMX_IN  OMX_PTR pCmdData);

    OMX_ERRORTYPE (*GetParameter)(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nParamIndex,  
            OMX_INOUT OMX_PTR pComponentParameterStructure);

    OMX_ERRORTYPE (*SetParameter)(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentParameterStructure);

    OMX_ERRORTYPE (*GetConfig)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_INOUT OMX_PTR pComponentConfigStructure);

    OMX_ERRORTYPE (*SetConfig)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_IN  OMX_PTR pComponentConfigStructure);

    OMX_ERRORTYPE (*GetExtensionIndex)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType);

    OMX_ERRORTYPE (*GetState)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STATETYPE* pState);

    OMX_ERRORTYPE (*ComponentTunnelRequest)(
        OMX_IN  OMX_HANDLETYPE hComp,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup); 

    OMX_ERRORTYPE (*UseBuffer)(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer);

    OMX_ERRORTYPE (*AllocateBuffer)(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes);

    OMX_ERRORTYPE (*FreeBuffer)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    OMX_ERRORTYPE (*EmptyThisBuffer)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    OMX_ERRORTYPE (*FillThisBuffer)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    OMX_ERRORTYPE (*SetCallbacks)(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_CALLBACKTYPE* pCallbacks, 
            OMX_IN  OMX_PTR pAppData);

    OMX_ERRORTYPE (*ComponentDeInit)(
            OMX_IN  OMX_HANDLETYPE hComponent);

    OMX_ERRORTYPE (*UseEGLImage)(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN void* eglImage);

    OMX_ERRORTYPE (*ComponentRoleEnum)(
        OMX_IN OMX_HANDLETYPE hComponent,
		OMX_OUT OMX_U8 *cRole,
		OMX_IN OMX_U32 nIndex);

} OMX_COMPONENTTYPE;

typedef struct OMX_CONFIG_COMMITMODETYPE { 
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_BOOL bDeferred; 
} OMX_CONFIG_COMMITMODETYPE;

typedef struct OMX_CONFIG_COMMITTYPE { 
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
} OMX_CONFIG_COMMITTYPE;

typedef enum OMX_MEDIACONTAINER_FORMATTYPE { 
    OMX_FORMAT_RAW = 0,
    OMX_FORMAT_MP4,
    OMX_FORMAT_3GP,
    OMX_FORMAT_3G2,
    OMX_FORMAT_AMC,
    OMX_FORMAT_SKM,
    OMX_FORMAT_K3G,
    OMX_FORMAT_VOB,
    OMX_FORMAT_AVI,
    OMX_FORMAT_ASF,
    OMX_FORMAT_RM,
    OMX_FORMAT_MPEG_ES,
    OMX_FORMAT_DIVX,
    OMX_FORMAT_MPEG_TS,
    OMX_FORMAT_QT,
    OMX_FORMAT_M4A,
    OMX_FORMAT_MP3,
    OMX_FORMAT_WAVE,
    OMX_FORMAT_XMF,
    OMX_FORMAT_AMR,
    OMX_FORMAT_AAC,
    OMX_FORMAT_EVRC,
    OMX_FORMAT_QCP,
    OMX_FORMAT_SMF,
    OMX_FORMAT_OGG,
    OMX_FORMAT_BMP,
    OMX_FORMAT_JPG,
    OMX_FORMAT_JPG2000,
    OMX_FORMAT_MKV,
    OMX_FORMAT_FLV,
    OMX_FORMAT_M4V,
    OMX_FORMAT_F4V,
    OMX_FORMAT_WEBM,
    OMX_FORMAT_WEBP,
    OMX_FORMATKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FORMATVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FORMATMax= 0x7FFFFFFF
} OMX_MEDIACONTAINER_FORMATTYPE;

typedef struct OMX_MEDIACONTAINER_INFOTYPE {
    OMX_U32  nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_MEDIACONTAINER_FORMATTYPE eFmtType;
} OMX_MEDIACONTAINER_INFOTYPE;

#define OMX_PORTSTATUS_ACCEPTUSEBUFFER      0x00000001
#define OMX_PORTSTATUS_ACCEPTBUFFEREXCHANGE 0x00000002

typedef struct OMX_CONFIG_TUNNELEDPORTSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nTunneledPortStatus;
} OMX_CONFIG_TUNNELEDPORTSTATUSTYPE;

typedef struct OMX_CONFIG_PORTBOOLEANTYPE{    
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnabled;
} OMX_CONFIG_PORTBOOLEANTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
