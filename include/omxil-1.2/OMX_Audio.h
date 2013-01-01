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
 *  OMX_Audio.h - OpenMax IL version 1.2.0
 *  The structures needed by Audio components to exchange
 *  parameters and configuration data with the componenmilts.
 */

#ifndef OMX_Audio_h
#define OMX_Audio_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header must include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully 
 */

#include <OMX_Core.h>

typedef enum OMX_AUDIO_CODINGTYPE {
    OMX_AUDIO_CodingUnused = 0,
    OMX_AUDIO_CodingAutoDetect,
    OMX_AUDIO_CodingPCM,
    OMX_AUDIO_CodingADPCM,
    OMX_AUDIO_CodingAMR,
    OMX_AUDIO_CodingGSMFR,
    OMX_AUDIO_CodingGSMEFR,
    OMX_AUDIO_CodingGSMHR,
    OMX_AUDIO_CodingPDCFR,
    OMX_AUDIO_CodingPDCEFR,
    OMX_AUDIO_CodingPDCHR,
    OMX_AUDIO_CodingTDMAFR,
    OMX_AUDIO_CodingTDMAEFR,
    OMX_AUDIO_CodingQCELP8,
    OMX_AUDIO_CodingQCELP13,
    OMX_AUDIO_CodingEVRC,
    OMX_AUDIO_CodingSMV,
    OMX_AUDIO_CodingG711,
    OMX_AUDIO_CodingG723,
    OMX_AUDIO_CodingG726,
    OMX_AUDIO_CodingG729,
    OMX_AUDIO_CodingAAC,
    OMX_AUDIO_CodingMP3,
    OMX_AUDIO_CodingSBC,
    OMX_AUDIO_CodingVORBIS,
    OMX_AUDIO_CodingWMA,
    OMX_AUDIO_CodingRA,
    OMX_AUDIO_CodingMIDI,
    OMX_AUDIO_CodingKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_CodingVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_CodingMax = 0x7FFFFFFF
} OMX_AUDIO_CODINGTYPE;

typedef struct OMX_AUDIO_PORTDEFINITIONTYPE {
    OMX_NATIVE_DEVICETYPE pNativeRender;
    OMX_BOOL bFlagErrorConcealment;
    OMX_AUDIO_CODINGTYPE eEncoding;
} OMX_AUDIO_PORTDEFINITIONTYPE;

typedef struct OMX_AUDIO_PARAM_PORTFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_AUDIO_CODINGTYPE eEncoding;
} OMX_AUDIO_PARAM_PORTFORMATTYPE;

typedef enum OMX_AUDIO_PCMMODETYPE { 
    OMX_AUDIO_PCMModeLinear = 0,
    OMX_AUDIO_PCMModeALaw,
    OMX_AUDIO_PCMModeMULaw,
    OMX_AUDIO_PCMModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_PCMModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_PCMModeMax = 0x7FFFFFFF 
} OMX_AUDIO_PCMMODETYPE; 

typedef enum OMX_AUDIO_CHANNELTYPE {
    OMX_AUDIO_ChannelNone = 0x0,
    OMX_AUDIO_ChannelLF   = 0x1,
    OMX_AUDIO_ChannelRF   = 0x2,
    OMX_AUDIO_ChannelCF   = 0x3,
    OMX_AUDIO_ChannelLS   = 0x4,
    OMX_AUDIO_ChannelRS   = 0x5,
    OMX_AUDIO_ChannelLFE  = 0x6,
    OMX_AUDIO_ChannelCS   = 0x7,
    OMX_AUDIO_ChannelLR   = 0x8,
    OMX_AUDIO_ChannelRR   = 0x9,
    OMX_AUDIO_ChannelLCF  = 0xA,
    OMX_AUDIO_ChannelRCF  = 0xB,
    OMX_AUDIO_ChannelLHS  = 0xC,
    OMX_AUDIO_ChannelRHS  = 0xD,
    OMX_AUDIO_ChannelCT   = 0xE,
    OMX_AUDIO_ChannelFLT  = 0xF,
    OMX_AUDIO_ChannelFCT  = 0x10,
    OMX_AUDIO_ChannelFRT  = 0x11,
    OMX_AUDIO_ChannelBLT  = 0x12,
    OMX_AUDIO_ChannelBCT  = 0x13,
    OMX_AUDIO_ChannelBRT  = 0x14,
    OMX_AUDIO_ChannelUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_ChannelKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_ChannelVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_ChannelMax  = 0x7FFFFFFF 
} OMX_AUDIO_CHANNELTYPE;

#define OMX_AUDIO_MAXCHANNELS   36
#define OMX_MIN_PCMPAYLOAD_MSEC 5

typedef struct OMX_AUDIO_PARAM_PCMMODETYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_NUMERICALDATATYPE eNumData;
    OMX_ENDIANTYPE eEndian;
    OMX_BOOL bInterleaved;
    OMX_U32 nBitPerSample;
    OMX_U32 nSamplingRate;
    OMX_AUDIO_PCMMODETYPE ePCMMode;
    OMX_AUDIO_CHANNELTYPE eChannelMapping[OMX_AUDIO_MAXCHANNELS];
} OMX_AUDIO_PARAM_PCMMODETYPE; 

typedef enum OMX_AUDIO_CHANNELMODETYPE {
    OMX_AUDIO_ChannelModeStereo = 0,
    OMX_AUDIO_ChannelModeJointStereo,
    OMX_AUDIO_ChannelModeDual,
    OMX_AUDIO_ChannelModeMono,
    OMX_AUDIO_ChannelModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_ChannelModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_ChannelModeMax = 0x7FFFFFFF
} OMX_AUDIO_CHANNELMODETYPE;

typedef enum OMX_AUDIO_MP3STREAMFORMATTYPE {
    OMX_AUDIO_MP3StreamFormatMP1Layer3 = 0,
    OMX_AUDIO_MP3StreamFormatMP2Layer3,
    OMX_AUDIO_MP3StreamFormatMP2_5Layer3,
    OMX_AUDIO_MP3StreamFormatUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_MP3StreamFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_MP3StreamFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_MP3StreamFormatMax = 0x7FFFFFFF
} OMX_AUDIO_MP3STREAMFORMATTYPE;

typedef struct OMX_AUDIO_PARAM_MP3TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_U32 nSampleRate;
    OMX_U32 nAudioBandWidth;
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
    OMX_AUDIO_MP3STREAMFORMATTYPE eFormat;
} OMX_AUDIO_PARAM_MP3TYPE;

typedef enum OMX_AUDIO_AACSTREAMFORMATTYPE {
    OMX_AUDIO_AACStreamFormatMP2ADTS = 0,
    OMX_AUDIO_AACStreamFormatMP4ADTS,
    OMX_AUDIO_AACStreamFormatMP4LOAS,
    OMX_AUDIO_AACStreamFormatMP4LATM,
    OMX_AUDIO_AACStreamFormatADIF,
    OMX_AUDIO_AACStreamFormatMP4FF,
    OMX_AUDIO_AACStreamFormatRAW,
    OMX_AUDIO_AACStreamFormatUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_AACStreamFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AACStreamFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AACStreamFormatMax = 0x7FFFFFFF
} OMX_AUDIO_AACSTREAMFORMATTYPE;

typedef enum OMX_AUDIO_AACPROFILETYPE{
    OMX_AUDIO_AACObjectNull = 0,
    OMX_AUDIO_AACObjectMain = 1,
    OMX_AUDIO_AACObjectLC,
    OMX_AUDIO_AACObjectSSR,
    OMX_AUDIO_AACObjectLTP,
    OMX_AUDIO_AACObjectHE,
    OMX_AUDIO_AACObjectScalable,
    OMX_AUDIO_AACObjectERLC = 17,
    OMX_AUDIO_AACObjectLD = 23,
    OMX_AUDIO_AACObjectHE_PS = 29,
    OMX_AUDIO_AACObjectUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_AACObjectKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AACObjectVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AACObjectMax = 0x7FFFFFFF
} OMX_AUDIO_AACPROFILETYPE;

#define OMX_AUDIO_AACToolNone 0x00000000
#define OMX_AUDIO_AACToolMS   0x00000001
#define OMX_AUDIO_AACToolIS   0x00000002
#define OMX_AUDIO_AACToolTNS  0x00000004
#define OMX_AUDIO_AACToolPNS  0x00000008
#define OMX_AUDIO_AACToolLTP  0x00000010
#define OMX_AUDIO_AACToolAll  0x7FFFFFFF

#define OMX_AUDIO_AACERNone  0x00000000
#define OMX_AUDIO_AACERVCB11 0x00000001
#define OMX_AUDIO_AACERRVLC  0x00000002
#define OMX_AUDIO_AACERHCR   0x00000004
#define OMX_AUDIO_AACERAll   0x7FFFFFFF

typedef struct OMX_AUDIO_PARAM_AACPROFILETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nSampleRate;
    OMX_U32 nBitRate;
    OMX_U32 nAudioBandWidth;
    OMX_U32 nFrameLength;
    OMX_U32 nAACtools;
    OMX_U32 nAACERtools;
    OMX_AUDIO_AACPROFILETYPE eAACProfile;
    OMX_AUDIO_AACSTREAMFORMATTYPE eAACStreamFormat;
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
} OMX_AUDIO_PARAM_AACPROFILETYPE;

typedef struct OMX_AUDIO_PARAM_VORBISTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_U32 nMinBitRate;
    OMX_U32 nMaxBitRate;
    OMX_U32 nSampleRate;
    OMX_U32 nAudioBandWidth;
    OMX_S32 nQuality;
    OMX_BOOL bManaged;
    OMX_BOOL bDownmix;
} OMX_AUDIO_PARAM_VORBISTYPE;

typedef enum OMX_AUDIO_WMAFORMATTYPE {
    OMX_AUDIO_WMAFormatUnused = 0,
    OMX_AUDIO_WMAFormat7,
    OMX_AUDIO_WMAFormat8,
    OMX_AUDIO_WMAFormat9,
    OMX_AUDIO_WMAFormat9_Professional,
    OMX_AUDIO_WMAFormat9_Lossless,
    OMX_AUDIO_WMAFormat9_Voice,
    OMX_AUDIO_WMAFormat10_Professional,
    OMX_AUDIO_WMAFormat10_Voice,
    OMX_AUDIO_WMAFormatUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_WMAFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_WMAFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_WMAFormatMax = 0x7FFFFFFF
} OMX_AUDIO_WMAFORMATTYPE;

typedef enum OMX_AUDIO_WMAPROFILETYPE {
    OMX_AUDIO_WMAProfileUnused = 0,
    OMX_AUDIO_WMAProfileL1,
    OMX_AUDIO_WMAProfileL2,
    OMX_AUDIO_WMAProfileL3,
    OMX_AUDIO_WMAProfileM0,
    OMX_AUDIO_WMAProfileM1,
    OMX_AUDIO_WMAProfileM2,
    OMX_AUDIO_WMAProfileM3,
    OMX_AUDIO_WMAProfileN1,
    OMX_AUDIO_WMAProfileN2,
    OMX_AUDIO_WMAProfileN3,
    OMX_AUDIO_WMAProfileS1,
    OMX_AUDIO_WMAProfileS2,
    OMX_AUDIO_WMAProfileUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_WMAProfileKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_WMAProfileVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_WMAProfileMax = 0x7FFFFFFF
} OMX_AUDIO_WMAPROFILETYPE;

typedef struct OMX_AUDIO_PARAM_WMATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U16 nChannels;
    OMX_U32 nBitRate; 
    OMX_AUDIO_WMAFORMATTYPE eFormat;
	OMX_AUDIO_WMAPROFILETYPE eProfile;
    OMX_U32 nSamplingRate;
    OMX_U16 nBlockAlign;
    OMX_U16 nEncodeOptions;
    OMX_U32 nSuperBlockAlign;
    OMX_U32 nBitsPerSample;
    OMX_U32 nAdvancedEncodeOpt;
    OMX_U32 nAdvancedEncodeOpt2;
} OMX_AUDIO_PARAM_WMATYPE;

typedef enum OMX_AUDIO_RAFORMATTYPE {
    OMX_AUDIO_RAFormatUnused = 0,
    OMX_AUDIO_RA8,
    OMX_AUDIO_RA9,
    OMX_AUDIO_RA10_AAC,
    OMX_AUDIO_RA10_CODEC,
    OMX_AUDIO_RA10_LOSSLESS,
    OMX_AUDIO_RA10_MULTICHANNEL,
    OMX_AUDIO_RA10_VOICE,
    OMX_AUDIO_RAFormatUnknown           = 0x6EFFFFFF,
    OMX_AUDIO_RAFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_RAFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_VIDEO_RAFormatMax = 0x7FFFFFFF
} OMX_AUDIO_RAFORMATTYPE;

typedef struct OMX_AUDIO_PARAM_RATYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nSamplingRate;
    OMX_U32 nBitsPerFrame;
    OMX_U32 nSamplePerFrame;
    OMX_U32 nCouplingQuantBits;
    OMX_U32 nCouplingStartRegion;
    OMX_U32 nNumRegions;
    OMX_AUDIO_RAFORMATTYPE eFormat;
} OMX_AUDIO_PARAM_RATYPE; 

typedef enum OMX_AUDIO_SBCALLOCMETHODTYPE {
  OMX_AUDIO_SBCAllocMethodLoudness,
  OMX_AUDIO_SBCAllocMethodSNR,
  OMX_AUDIO_SBCAllocMethodKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
  OMX_AUDIO_SBCAllocMethodVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
  OMX_AUDIO_SBCAllocMethodMax = 0x7FFFFFFF
} OMX_AUDIO_SBCALLOCMETHODTYPE;

typedef struct OMX_AUDIO_PARAM_SBCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_U32 nSampleRate;
    OMX_U32 nBlocks;
    OMX_U32 nSubbands;
    OMX_U32 nBitPool;
    OMX_BOOL bEnableBitrate;
    OMX_AUDIO_CHANNELMODETYPE eChannelMode;
    OMX_AUDIO_SBCALLOCMETHODTYPE eSBCAllocType;
} OMX_AUDIO_PARAM_SBCTYPE;

typedef struct OMX_AUDIO_PARAM_ADPCMTYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitsPerSample;
    OMX_U32 nSampleRate;
    OMX_U32 nBlockSize;
} OMX_AUDIO_PARAM_ADPCMTYPE; 

typedef enum OMX_AUDIO_G723RATE {
    OMX_AUDIO_G723ModeUnused = 0,
    OMX_AUDIO_G723ModeLow,
    OMX_AUDIO_G723ModeHigh,
    OMX_AUDIO_G723ModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_G723ModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_G723ModeMax = 0x7FFFFFFF
} OMX_AUDIO_G723RATE;

typedef struct OMX_AUDIO_PARAM_G723TYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_AUDIO_G723RATE eBitRate;
    OMX_BOOL bHiPassFilter;
    OMX_BOOL bPostFilter;
} OMX_AUDIO_PARAM_G723TYPE; 

typedef enum OMX_AUDIO_G726MODE {
    OMX_AUDIO_G726ModeUnused = 0,
    OMX_AUDIO_G726Mode16,
    OMX_AUDIO_G726Mode24,
    OMX_AUDIO_G726Mode32,
    OMX_AUDIO_G726Mode40,
    OMX_AUDIO_G726ModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_G726ModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_G726ModeMax = 0x7FFFFFFF
} OMX_AUDIO_G726MODE;

typedef struct OMX_AUDIO_PARAM_G726TYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
     OMX_AUDIO_G726MODE eG726Mode;
} OMX_AUDIO_PARAM_G726TYPE; 

typedef enum OMX_AUDIO_G729TYPE {
    OMX_AUDIO_G729 = 0,
    OMX_AUDIO_G729A,
    OMX_AUDIO_G729B,
    OMX_AUDIO_G729AB,
    OMX_AUDIO_G729KhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_G729VendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_G729Max = 0x7FFFFFFF
} OMX_AUDIO_G729TYPE;

typedef struct OMX_AUDIO_PARAM_G729TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_AUDIO_G729TYPE eBitType;
} OMX_AUDIO_PARAM_G729TYPE;

typedef enum OMX_AUDIO_AMRFRAMEFORMATTYPE { 
    OMX_AUDIO_AMRFrameFormatConformance = 0,
    OMX_AUDIO_AMRFrameFormatIF1,
    OMX_AUDIO_AMRFrameFormatIF2,
    OMX_AUDIO_AMRFrameFormatFSF,
    OMX_AUDIO_AMRFrameFormatRTPPayloadFull,
    OMX_AUDIO_AMRFrameFormatITU,
    OMX_AUDIO_AMRFrameFormatRTPPayloadConstrained,
    OMX_AUDIO_AMRFrameFormatWBPlusTIF,
    OMX_AUDIO_AMRFrameFormatWBPlusFSF,
    OMX_AUDIO_AMRFrameFormatWBPlusRTPPayloadBasic,
    OMX_AUDIO_AMRFrameFormatWBPlusRTPPayloadInterleaved,
    OMX_AUDIO_AMRFrameFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AMRFrameFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AMRFrameFormatMax = 0x7FFFFFFF 
} OMX_AUDIO_AMRFRAMEFORMATTYPE; 

typedef enum OMX_AUDIO_AMRBANDMODETYPE {
    OMX_AUDIO_AMRBandModeUnused = 0,
    OMX_AUDIO_AMRBandModeNB0,
    OMX_AUDIO_AMRBandModeNB1,
    OMX_AUDIO_AMRBandModeNB2,
    OMX_AUDIO_AMRBandModeNB3,
    OMX_AUDIO_AMRBandModeNB4,
    OMX_AUDIO_AMRBandModeNB5,
    OMX_AUDIO_AMRBandModeNB6,
    OMX_AUDIO_AMRBandModeNB7,
    OMX_AUDIO_AMRBandModeWB0,
    OMX_AUDIO_AMRBandModeWB1,
    OMX_AUDIO_AMRBandModeWB2,
    OMX_AUDIO_AMRBandModeWB3,
    OMX_AUDIO_AMRBandModeWB4,
    OMX_AUDIO_AMRBandModeWB5,
    OMX_AUDIO_AMRBandModeWB6,
    OMX_AUDIO_AMRBandModeWB7,
    OMX_AUDIO_AMRBandModeWB8,
    OMX_AUDIO_AMRBandModeWBP0,
    OMX_AUDIO_AMRBandModeWBP1,
    OMX_AUDIO_AMRBandModeWBP2,
    OMX_AUDIO_AMRBandModeWBP3,
    OMX_AUDIO_AMRBandModeWBP4,
    OMX_AUDIO_AMRBandModeWBP5,
    OMX_AUDIO_AMRBandModeWBP6,
    OMX_AUDIO_AMRBandModeWBP7,
    OMX_AUDIO_AMRBandModeWBP8,
    OMX_AUDIO_AMRBandModeWBP9,
    OMX_AUDIO_AMRBandModeWBP10,
    OMX_AUDIO_AMRBandModeWBP11,
    OMX_AUDIO_AMRBandModeWBP12,
    OMX_AUDIO_AMRBandModeWBP13,
    OMX_AUDIO_AMRBandModeWBP14,
    OMX_AUDIO_AMRBandModeWBP15,
    OMX_AUDIO_AMRBandModeWBP16,
    OMX_AUDIO_AMRBandModeWBP17,
    OMX_AUDIO_AMRBandModeWBP18,
    OMX_AUDIO_AMRBandModeWBP19,
    OMX_AUDIO_AMRBandModeWBP20,
    OMX_AUDIO_AMRBandModeWBP21,
    OMX_AUDIO_AMRBandModeWBP22,
    OMX_AUDIO_AMRBandModeWBP23,
    OMX_AUDIO_AMRBandModeWBP24,
    OMX_AUDIO_AMRBandModeWBP25,
    OMX_AUDIO_AMRBandModeWBP26,
    OMX_AUDIO_AMRBandModeWBP27,
    OMX_AUDIO_AMRBandModeWBP28,
    OMX_AUDIO_AMRBandModeWBP29,
    OMX_AUDIO_AMRBandModeWBP30,
    OMX_AUDIO_AMRBandModeWBP31,
    OMX_AUDIO_AMRBandModeWBP32,
    OMX_AUDIO_AMRBandModeWBP33,
    OMX_AUDIO_AMRBandModeWBP34,
    OMX_AUDIO_AMRBandModeWBP35,
    OMX_AUDIO_AMRBandModeWBP36,
    OMX_AUDIO_AMRBandModeWBP37,
    OMX_AUDIO_AMRBandModeWBP38,
    OMX_AUDIO_AMRBandModeWBP39,
    OMX_AUDIO_AMRBandModeWBP40,
    OMX_AUDIO_AMRBandModeWBP41,
    OMX_AUDIO_AMRBandModeWBP42,
    OMX_AUDIO_AMRBandModeWBP43,
    OMX_AUDIO_AMRBandModeWBP44,
    OMX_AUDIO_AMRBandModeWBP45,
    OMX_AUDIO_AMRBandModeWBP46,
    OMX_AUDIO_AMRBandModeWBP47,
    OMX_AUDIO_AMRBandModeAuto,
    OMX_AUDIO_AMRBandModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AMRBandModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AMRBandModeMax = 0x7FFFFFFF
} OMX_AUDIO_AMRBANDMODETYPE;
     
typedef enum OMX_AUDIO_AMRDTXMODETYPE { 
    OMX_AUDIO_AMRDTXModeOff = 0,
    OMX_AUDIO_AMRDTXModeOnVAD1,
    OMX_AUDIO_AMRDTXModeOnVAD2,
    OMX_AUDIO_AMRDTXModeOnAuto,
    OMX_AUDIO_AMRDTXasEFR,
    OMX_AUDIO_AMRDTXModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AMRDTXModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AMRDTXModeMax = 0x7FFFFFFF 
} OMX_AUDIO_AMRDTXMODETYPE; 
 
typedef enum OMX_AUDIO_AMRISFINDEXTYPE {
    OMX_AUDIO_AMRISFIndex0,
    OMX_AUDIO_AMRISFIndex1,
    OMX_AUDIO_AMRISFIndex2,
    OMX_AUDIO_AMRISFIndex3,
    OMX_AUDIO_AMRISFIndex4,
    OMX_AUDIO_AMRISFIndex5,
    OMX_AUDIO_AMRISFIndex6,
    OMX_AUDIO_AMRISFIndex7,
    OMX_AUDIO_AMRISFIndex8,
    OMX_AUDIO_AMRISFIndex9,
    OMX_AUDIO_AMRISFIndex10,
    OMX_AUDIO_AMRISFIndex11,
    OMX_AUDIO_AMRISFIndex12,
    OMX_AUDIO_AMRISFIndex13,
    OMX_AUDIO_AMRISFIndexAuto,
    OMX_AUDIO_AMRISFIndexUknown,
    OMX_AUDIO_AMRISFIndexKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_AMRISFIndexVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_AMRISFIndexMax = 0x7FFFFFFF
} OMX_AUDIO_AMRISFINDEXTYPE;

typedef struct OMX_AUDIO_PARAM_AMRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_AUDIO_AMRBANDMODETYPE eAMRBandMode;
    OMX_AUDIO_AMRDTXMODETYPE  eAMRDTXMode;
    OMX_AUDIO_AMRFRAMEFORMATTYPE eAMRFrameFormat;
    OMX_AUDIO_AMRISFINDEXTYPE eAMRISFIndex;
} OMX_AUDIO_PARAM_AMRTYPE;

typedef struct OMX_AUDIO_PARAM_GSMFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_GSMFRTYPE;

typedef struct OMX_AUDIO_PARAM_GSMHRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_GSMHRTYPE;

typedef struct OMX_AUDIO_PARAM_GSMEFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_GSMEFRTYPE;

typedef struct OMX_AUDIO_PARAM_TDMAFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_TDMAFRTYPE;

typedef struct OMX_AUDIO_PARAM_TDMAEFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_TDMAEFRTYPE;

typedef struct OMX_AUDIO_PARAM_PDCFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_PDCFRTYPE;

typedef struct OMX_AUDIO_PARAM_PDCEFRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_PDCEFRTYPE;

typedef struct OMX_AUDIO_PARAM_PDCHRTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_BOOL bDTX;
    OMX_BOOL bHiPassFilter;
} OMX_AUDIO_PARAM_PDCHRTYPE;

typedef enum OMX_AUDIO_CDMARATETYPE {
    OMX_AUDIO_CDMARateBlank = 0,
    OMX_AUDIO_CDMARateFull,
    OMX_AUDIO_CDMARateHalf,
    OMX_AUDIO_CDMARateQuarter,
    OMX_AUDIO_CDMARateEighth,
    OMX_AUDIO_CDMARateErasure,
    OMX_AUDIO_CDMARateKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_CDMARateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_CDMARateMax = 0x7FFFFFFF
} OMX_AUDIO_CDMARATETYPE;

typedef struct OMX_AUDIO_PARAM_QCELP8TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_U32 nBitRate;
    OMX_AUDIO_CDMARATETYPE eCDMARate;
    OMX_U32 nMinBitRate;
    OMX_U32 nMaxBitRate;
} OMX_AUDIO_PARAM_QCELP8TYPE;

typedef struct OMX_AUDIO_PARAM_QCELP13TYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_AUDIO_CDMARATETYPE eCDMARate;
    OMX_U32 nMinBitRate;
    OMX_U32 nMaxBitRate;
} OMX_AUDIO_PARAM_QCELP13TYPE;

typedef struct OMX_AUDIO_PARAM_EVRCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_AUDIO_CDMARATETYPE eCDMARate;
    OMX_BOOL bRATE_REDUCon;
    OMX_U32 nMinBitRate;
    OMX_U32 nMaxBitRate;
    OMX_BOOL bHiPassFilter;
    OMX_BOOL bNoiseSuppressor;
    OMX_BOOL bPostFilter;
} OMX_AUDIO_PARAM_EVRCTYPE;

typedef struct OMX_AUDIO_PARAM_SMVTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_AUDIO_CDMARATETYPE eCDMARate;
    OMX_BOOL bRATE_REDUCon;
    OMX_U32 nMinBitRate;
    OMX_U32 nMaxBitRate;
    OMX_BOOL bHiPassFilter;
    OMX_BOOL bNoiseSuppressor;
    OMX_BOOL bPostFilter;
} OMX_AUDIO_PARAM_SMVTYPE;

typedef enum OMX_AUDIO_MIDIFORMATTYPE
{
    OMX_AUDIO_MIDIFormatUnknown = 0,
    OMX_AUDIO_MIDIFormatSMF0,
    OMX_AUDIO_MIDIFormatSMF1,
    OMX_AUDIO_MIDIFormatSMF2,
    OMX_AUDIO_MIDIFormatSPMIDI,
    OMX_AUDIO_MIDIFormatXMF0,
    OMX_AUDIO_MIDIFormatXMF1,
    OMX_AUDIO_MIDIFormatMobileXMF,
    OMX_AUDIO_MIDIFormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_MIDIFormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_MIDIFormatMax = 0x7FFFFFFF
} OMX_AUDIO_MIDIFORMATTYPE;

typedef struct OMX_AUDIO_PARAM_MIDITYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFileSize;
    OMX_BU32 sMaxPolyphony;
    OMX_BOOL bLoadDefaultSound;
    OMX_AUDIO_MIDIFORMATTYPE eMidiFormat;
} OMX_AUDIO_PARAM_MIDITYPE;

typedef enum OMX_AUDIO_MIDISOUNDBANKTYPE {
    OMX_AUDIO_MIDISoundBankUnused = 0,
    OMX_AUDIO_MIDISoundBankDLS1,
    OMX_AUDIO_MIDISoundBankDLS2,
    OMX_AUDIO_MIDISoundBankMobileDLSBase,
    OMX_AUDIO_MIDISoundBankMobileDLSPlusOptions,
    OMX_AUDIO_MIDISoundBankKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_MIDISoundBankVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_MIDISoundBankMax = 0x7FFFFFFF
} OMX_AUDIO_MIDISOUNDBANKTYPE;

typedef enum OMX_AUDIO_MIDISOUNDBANKLAYOUTTYPE {
   OMX_AUDIO_MIDISoundBankLayoutUnused = 0,
   OMX_AUDIO_MIDISoundBankLayoutGM,
   OMX_AUDIO_MIDISoundBankLayoutGM2,
   OMX_AUDIO_MIDISoundBankLayoutUser,
   OMX_AUDIO_MIDISoundBankLayoutKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
   OMX_AUDIO_MIDISoundBankLayoutVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
   OMX_AUDIO_MIDISoundBankLayoutMax = 0x7FFFFFFF
} OMX_AUDIO_MIDISOUNDBANKLAYOUTTYPE;

typedef struct OMX_AUDIO_PARAM_MIDILOADUSERSOUNDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nDLSIndex;
    OMX_U32 nDLSSize;
    OMX_PTR pDLSData;
    OMX_AUDIO_MIDISOUNDBANKTYPE eMidiSoundBank;
    OMX_AUDIO_MIDISOUNDBANKLAYOUTTYPE eMidiSoundBankLayout;
} OMX_AUDIO_PARAM_MIDILOADUSERSOUNDTYPE;

typedef struct OMX_AUDIO_CONFIG_MIDIIMMEDIATEEVENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nMidiEventSize;
    OMX_U8 nMidiEvents[1];
} OMX_AUDIO_CONFIG_MIDIIMMEDIATEEVENTTYPE;

typedef struct OMX_AUDIO_CONFIG_MIDISOUNDBANKPROGRAMTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannel;
    OMX_U16 nIDProgram;
    OMX_U16 nIDSoundBank;
    OMX_U32 nUserSoundBankIndex;
} OMX_AUDIO_CONFIG_MIDISOUNDBANKPROGRAMTYPE;

typedef struct OMX_AUDIO_CONFIG_MIDICONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BS32 sPitchTransposition;
    OMX_BU32 sPlayBackRate;
    OMX_BU32 sTempo;
    OMX_U32 nMaxPolyphony;
    OMX_U32 nNumRepeat;
    OMX_U32 nStopTime;
    OMX_U16 nChannelMuteMask;
    OMX_U16 nChannelSoloMask;
    OMX_U32 nTrack0031MuteMask;
    OMX_U32 nTrack3263MuteMask;
    OMX_U32 nTrack0031SoloMask;
    OMX_U32 nTrack3263SoloMask;
} OMX_AUDIO_CONFIG_MIDICONTROLTYPE;

typedef enum OMX_AUDIO_MIDIPLAYBACKSTATETYPE {
  OMX_AUDIO_MIDIPlayBackStateUnknown = 0,
  OMX_AUDIO_MIDIPlayBackStateClosedEngaged,
  OMX_AUDIO_MIDIPlayBackStateParsing,
  OMX_AUDIO_MIDIPlayBackStateOpenEngaged,
  OMX_AUDIO_MIDIPlayBackStatePlaying,
  OMX_AUDIO_MIDIPlayBackStatePlayingPartially,
  OMX_AUDIO_MIDIPlayBackStatePlayingSilently,
  OMX_AUDIO_MIDIPlayBackStateKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
  OMX_AUDIO_MIDIPlayBackStateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
  OMX_AUDIO_MIDIPlayBackStateMax = 0x7FFFFFFF
} OMX_AUDIO_MIDIPLAYBACKSTATETYPE;

typedef struct OMX_AUDIO_CONFIG_MIDISTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U16 nNumTracks;
    OMX_U32 nDuration;
    OMX_U32 nPosition;
    OMX_BOOL bVibra;
    OMX_U32 nNumMetaEvents;
    OMX_U32 nNumActiveVoices;
    OMX_AUDIO_MIDIPLAYBACKSTATETYPE eMIDIPlayBackState;
} OMX_AUDIO_CONFIG_MIDISTATUSTYPE;

typedef struct OMX_AUDIO_CONFIG_MIDIMETAEVENTTYPE{ 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_U8 nMetaEventType;
    OMX_U32 nMetaEventSize;
    OMX_U32 nTrack;
    OMX_U32 nPosition;
} OMX_AUDIO_CONFIG_MIDIMETAEVENTTYPE; 

typedef struct OMX_AUDIO_CONFIG_MIDIMETAEVENTDATATYPE{ 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_U32 nMetaEventSize;
    OMX_U8 nData[1];
} OMX_AUDIO_CONFIG__MIDIMETAEVENTDATATYPE; 

typedef struct OMX_AUDIO_CONFIG_VOLUMETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bLinear;
    OMX_BS32 sVolume;
} OMX_AUDIO_CONFIG_VOLUMETYPE;

typedef struct OMX_AUDIO_CONFIG_CHANNELVOLUMETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannel;
    OMX_BOOL bLinear;
    OMX_BS32 sVolume;
    OMX_BOOL bIsMIDI;
} OMX_AUDIO_CONFIG_CHANNELVOLUMETYPE;

typedef struct OMX_AUDIO_CONFIG_BALANCETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nBalance;
} OMX_AUDIO_CONFIG_BALANCETYPE;

typedef struct OMX_AUDIO_CONFIG_MUTETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bMute;
} OMX_AUDIO_CONFIG_MUTETYPE;

typedef struct OMX_AUDIO_CONFIG_CHANNELMUTETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannel;
    OMX_BOOL bMute;
    OMX_BOOL bIsMIDI;
} OMX_AUDIO_CONFIG_CHANNELMUTETYPE;

typedef struct OMX_AUDIO_CONFIG_LOUDNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bLoudness;
} OMX_AUDIO_CONFIG_LOUDNESSTYPE;

typedef struct OMX_AUDIO_CONFIG_BASSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_S32 nBass;
} OMX_AUDIO_CONFIG_BASSTYPE;

typedef struct OMX_AUDIO_CONFIG_TREBLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_S32  nTreble;
} OMX_AUDIO_CONFIG_TREBLETYPE;

typedef struct OMX_AUDIO_CONFIG_EQUALIZERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BU32 sBandIndex;
    OMX_BU32 sCenterFreq;
    OMX_BS32 sBandLevel;
} OMX_AUDIO_CONFIG_EQUALIZERTYPE;

typedef enum OMX_AUDIO_STEREOWIDENINGTYPE {
    OMX_AUDIO_StereoWideningHeadphones,
    OMX_AUDIO_StereoWideningLoudspeakers,
    OMX_AUDIO_StereoWideningKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AUDIO_StereoWideningVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AUDIO_StereoWideningMax = 0x7FFFFFFF
} OMX_AUDIO_STEREOWIDENINGTYPE;

typedef struct OMX_AUDIO_CONFIG_STEREOWIDENINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_AUDIO_STEREOWIDENINGTYPE eWideningType;
    OMX_U32  nStereoWidening;
} OMX_AUDIO_CONFIG_STEREOWIDENINGTYPE;

typedef struct OMX_AUDIO_CONFIG_CHORUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BU32 sDelay;
    OMX_BU32 sModulationRate;
    OMX_U32 nModulationDepth;
    OMX_BU32 nFeedback;
} OMX_AUDIO_CONFIG_CHORUSTYPE;

typedef struct OMX_AUDIO_CONFIG_REVERBERATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_BS32 sRoomLevel;
    OMX_BS32 sRoomHighFreqLevel;
    OMX_BS32 sReflectionsLevel;
    OMX_BU32 sReflectionsDelay;
    OMX_BS32 sReverbLevel;
    OMX_BU32 sReverbDelay;
    OMX_BU32 sDecayTime;
    OMX_BU32 nDecayHighFreqRatio;
    OMX_U32 nDensity;
    OMX_U32 nDiffusion;
    OMX_BU32 sReferenceHighFreq;
} OMX_AUDIO_CONFIG_REVERBERATIONTYPE;

typedef enum OMX_AUDIO_ECHOCANTYPE {
   OMX_AUDIO_EchoCanOff = 0,
   OMX_AUDIO_EchoCanNormal,
   OMX_AUDIO_EchoCanHFree,
   OMX_AUDIO_EchoCanCarKit,
   OMX_AUDIO_EchoCanKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
   OMX_AUDIO_EchoCanVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
   OMX_AUDIO_EchoCanMax = 0x7FFFFFFF
} OMX_AUDIO_ECHOCANTYPE;

typedef struct OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_AUDIO_ECHOCANTYPE eEchoCancelation;
} OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE;

typedef struct OMX_AUDIO_CONFIG_NOISEREDUCTIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bNoiseReduction;
} OMX_AUDIO_CONFIG_NOISEREDUCTIONTYPE;

typedef enum OMX_AUDIO_3DOUTPUTTYPE {
    OMX_AUDIO_3DOutputHeadphones,
    OMX_AUDIO_3DOutputLoudspeakers,
    OMX_AUDIO_3DOutputMax = 0x7FFFFFFF
} OMX_AUDIO_3DOUTPUTTYPE;

typedef struct OMX_AUDIO_CONFIG_3DOUTPUTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_AUDIO_3DOUTPUTTYPE e3DOutputType;
} OMX_AUDIO_CONFIG_3DOUTPUTTYPE;

typedef struct OMX_AUDIO_CONFIG_3DLOCATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nX;
    OMX_S32 nY;
    OMX_S32 nZ;
} OMX_AUDIO_CONFIG_3DLOCATIONTYPE;

typedef struct OMX_AUDIO_PARAM_3DDOPPLERMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnabled;
} OMX_AUDIO_PARAM_3DDOPPLERMODETYPE;

typedef struct OMX_AUDIO_CONFIG_3DDOPPLERSETTINGSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nSoundSpeed;
    OMX_S32 nSourceVelocity;
    OMX_S32 nListenerVelocity;
} OMX_AUDIO_CONFIG_3DDOPPLERSETTINGSTYPE;

typedef struct OMX_AUDIO_CONFIG_3DLEVELSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BS32 sDirectLevel;
    OMX_BS32 sRoomLevel;
} OMX_AUDIO_CONFIG_3DLEVELSTYPE;

typedef enum OMX_AUDIO_ROLLOFFMODELTYPE {
    OMX_AUDIO_RollOffExponential,
    OMX_AUDIO_RollOffLinear,
    OMX_AUDIO_RollOffMax = 0x7FFFFFFF
} OMX_AUDIO_ROLLOFFMODELTYPE;

typedef struct OMX_AUDIO_CONFIG_3DDISTANCEATTENUATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BS32 sMinDistance;
    OMX_BS32 sMaxDistance;
    OMX_BS32 sRollOffFactor;
    OMX_BS32 sRoomRollOffFactor;
    OMX_AUDIO_ROLLOFFMODELTYPE eRollOffModel;
    OMX_BOOL bMuteAfterMax;
} OMX_AUDIO_CONFIG_3DDISTANCEATTENUATIONTYPE;

typedef struct OMX_AUDIO_CONFIG_3DDIRECTIVITYSETTINGSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BS32 sInnerAngle;
    OMX_BS32 sOuterAngle;
    OMX_BS32 sOuterLevel;
} OMX_AUDIO_CONFIG_3DDIRECTIVITYSETTINGSTYPE;

typedef struct OMX_AUDIO_CONFIG_3DDIRECTIVITYORIENTATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nXFront;
    OMX_S32 nYFront;
    OMX_S32 nZFront;
} OMX_AUDIO_CONFIG_3DDIRECTIVITYORIENTATIONTYPE;

typedef struct OMX_AUDIO_CONFIG_3DMACROSCOPICORIENTATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nXFront;
    OMX_S32 nYFront;
    OMX_S32 nZFront;
    OMX_S32 nXAbove;
    OMX_S32 nYAbove;
    OMX_S32 nZAbove;
} OMX_AUDIO_CONFIG_3DMACROSCOPICORIENTATIONTYPE;

typedef struct OMX_AUDIO_CONFIG_3DMACROSCOPICSIZETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nWidth;
    OMX_S32 nHeight;
    OMX_S32 nDepth;
} OMX_AUDIO_CONFIG_3DMACROSCOPICSIZETYPE;

typedef struct OMX_AUDIO_PARAM_CHANNELMAPPINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nChannels;
    OMX_AUDIO_CHANNELTYPE nChannelsMapping[OMX_AUDIO_MAXCHANNELS];
} OMX_AUDIO_PARAM_CHANNELMAPPINGTYPE;

typedef struct OMX_AUDIO_SBCBITPOOLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nNewBitPool;
} OMX_AUDIO_SBCBITPOOLTYPE;

typedef struct OMX_AUDIO_AMRMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBitRate;
    OMX_AUDIO_AMRBANDMODETYPE eAMRBandMode;
} OMX_AUDIO_AMRMODETYPE;

typedef struct OMX_AUDIO_CONFIG_BITRATETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nEncodeBitrate;
} OMX_AUDIO_CONFIG_BITRATETYPE;

typedef struct OMX_AUDIO_CONFIG_AMRISFTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_AUDIO_AMRISFINDEXTYPE  eTargetAMRISFIndex;
} OMX_AUDIO_CONFIG_AMRISFTYPE;

typedef struct OMX_AUDIO_FIXEDPOINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nValueStartBit;
    OMX_U32 nValueBits;
    OMX_U32 nSignExtensionBits;
    OMX_S32 nValuePointPosition;
} OMX_AUDIO_FIXEDPOINTTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */

