/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   tizosalutils.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - OpenMAX IL types utility functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "OMX_Core.h"
#include "tizext.h"

#include "tizosalutils.h"


typedef struct tiz_cmd_str tiz_cmd_str_t;
struct tiz_cmd_str
{
  OMX_COMMANDTYPE cmd;
  OMX_STRING str;
};

static const tiz_cmd_str_t tiz_cmd_to_str_tbl[] = {
  {OMX_CommandStateSet, "OMX_CommandStateSet"},
  {OMX_CommandFlush, "OMX_CommandFlush"},
  {OMX_CommandPortDisable, "OMX_CommandPortDisable"},
  {OMX_CommandPortEnable, "OMX_CommandPortEnable"},
  {OMX_CommandMarkBuffer, "OMX_CommandMarkBuffer"},
  {OMX_CommandKhronosExtensions, "OMX_CommandKhronosExtensions"},
  {OMX_CommandVendorStartUnused, "OMX_CommandVendorStartUnused"},
  {OMX_CommandMax, "OMX_CommandMax"}
};

typedef struct tiz_evt_str tiz_evt_str_t;
struct tiz_evt_str
{
  OMX_EVENTTYPE evt;
  OMX_STRING str;
};

static const tiz_evt_str_t tiz_evt_to_str_tbl[] = {
  {OMX_EventCmdComplete, "OMX_EventCmdComplete"},
  {OMX_EventError, "OMX_EventError"},
  {OMX_EventMark, "OMX_EventMark"},
  {OMX_EventPortSettingsChanged, "OMX_EventPortSettingsChanged"},
  {OMX_EventBufferFlag, "OMX_EventBufferFlag"},
  {OMX_EventResourcesAcquired, "OMX_EventResourcesAcquired"},
  {OMX_EventComponentResumed, "OMX_EventComponentResumed"},
  {OMX_EventDynamicResourcesAvailable,  "OMX_EventDynamicResourcesAvailable"},
  {OMX_EventPortFormatDetected,  "OMX_EventPortFormatDetected"},
  {OMX_EventIndexSettingChanged,  "OMX_EventIndexSettingChanged"},
  {OMX_EventPortNeedsDisable,  "OMX_EventPortNeedsDisable"},
  {OMX_EventPortNeedsFlush,  "OMX_EventPortNeedsFlush"},
  {OMX_EventKhronosExtensions, "OMX_EventKhronosExtensions"},
  {OMX_EventVendorStartUnused, "OMX_EventVendorStartUnused"},
  {OMX_EventMax, "OMX_EventMax"}
};

typedef struct tiz_state_str tiz_state_str_t;

struct tiz_state_str
{
  OMX_STATETYPE state;
  OMX_STRING str;
};

static const tiz_state_str_t tiz_state_to_str_tbl[] = {
  {OMX_StateReserved_0x00000000, "OMX_StateReserved_0x00000000"},
  {OMX_StateLoaded, "OMX_StateLoaded"},
  {OMX_StateIdle, "OMX_StateIdle"},
  {OMX_StateExecuting, "OMX_StateExecuting"},
  {OMX_StatePause, "OMX_StatePause"},
  {OMX_StateWaitForResources, "OMX_StateWaitForResources"},
  {OMX_StateKhronosExtensions, "OMX_StateKhronosExtensions"},
  {OMX_StateVendorStartUnused, "OMX_StateVendorStartUnused"},
  {OMX_StateMax, "OMX_StateMax"}
};

typedef struct tiz_dir_str tiz_dir_str_t;
struct tiz_dir_str
{
  OMX_DIRTYPE dir;
  OMX_STRING str;
};

static const tiz_dir_str_t tiz_dir_to_str_tbl[] = {
  {OMX_DirInput, "OMX_DirInput"},
  {OMX_DirOutput, "OMX_DirOutput"},
  {OMX_DirMax, "OMX_DirMax"}
};

typedef struct tiz_err_str tiz_err_str_t;
struct tiz_err_str
{
  OMX_ERRORTYPE err;
  OMX_STRING str;
};

static const tiz_err_str_t tiz_err_to_str_tbl[] = {
  {OMX_ErrorNone, "OMX_ErrorNone"},
  {OMX_ErrorInsufficientResources, "OMX_ErrorInsufficientResources"},
  {OMX_ErrorUndefined, "OMX_ErrorUndefined"},
  {OMX_ErrorInvalidComponentName, "OMX_ErrorInvalidComponentName"},
  {OMX_ErrorComponentNotFound, "OMX_ErrorComponentNotFound"},
  {OMX_ErrorReserved_0x80001004, "OMX_ErrorReserved_0x80001004"},
  {OMX_ErrorBadParameter, "OMX_ErrorBadParameter"},
  {OMX_ErrorNotImplemented, "OMX_ErrorNotImplemented"},
  {OMX_ErrorUnderflow, "OMX_ErrorUnderflow"},
  {OMX_ErrorOverflow, "OMX_ErrorOverflow"},
  {OMX_ErrorHardware, "OMX_ErrorHardware"},
  {OMX_ErrorReserved_0x8000100A, "OMX_ErrorReserved_0x8000100A"},
  {OMX_ErrorStreamCorrupt, "OMX_ErrorStreamCorrupt"},
  {OMX_ErrorPortsNotCompatible, "OMX_ErrorPortsNotCompatible"},
  {OMX_ErrorResourcesLost, "OMX_ErrorResourcesLost"},
  {OMX_ErrorNoMore, "OMX_ErrorNoMore"},
  {OMX_ErrorVersionMismatch, "OMX_ErrorVersionMismatch"},
  {OMX_ErrorNotReady, "OMX_ErrorNotReady"},
  {OMX_ErrorTimeout, "OMX_ErrorTimeout"},
  {OMX_ErrorSameState, "OMX_ErrorSameState"},
  {OMX_ErrorResourcesPreempted, "OMX_ErrorResourcesPreempted"},
  {OMX_ErrorReserved_0x80001014,
   "OMX_ErrorReserved_0x80001014"},
  {OMX_ErrorReserved_0x80001015,
   "OMX_ErrorReserved_0x80001015"},
  {OMX_ErrorReserved_0x80001016,
   "OMX_ErrorReserved_0x80001016"},
  {OMX_ErrorIncorrectStateTransition, "OMX_ErrorIncorrectStateTransition"},
  {OMX_ErrorIncorrectStateOperation, "OMX_ErrorIncorrectStateOperation"},
  {OMX_ErrorUnsupportedSetting, "OMX_ErrorUnsupportedSetting"},
  {OMX_ErrorUnsupportedIndex, "OMX_ErrorUnsupportedIndex"},
  {OMX_ErrorBadPortIndex, "OMX_ErrorBadPortIndex"},
  {OMX_ErrorPortUnpopulated, "OMX_ErrorPortUnpopulated"},
  {OMX_ErrorComponentSuspended, "OMX_ErrorComponentSuspended"},
  {OMX_ErrorDynamicResourcesUnavailable,
   "OMX_ErrorDynamicResourcesUnavailable"},
  {OMX_ErrorMbErrorsInFrame, "OMX_ErrorMbErrorsInFrame"},
  {OMX_ErrorFormatNotDetected, "OMX_ErrorFormatNotDetected"},
  {OMX_ErrorReserved_0x80001021, "OMX_ErrorReserved_0x80001021"},
  {OMX_ErrorReserved_0x80001022, "OMX_ErrorReserved_0x80001022"},
  {OMX_ErrorSeperateTablesUsed, "OMX_ErrorSeperateTablesUsed"},
  {OMX_ErrorTunnelingUnsupported, "OMX_ErrorTunnelingUnsupported"},
  {OMX_ErrorInvalidMode, "OMX_ErrorInvalidMode"},
  {OMX_ErrorStreamCorruptStalled, "OMX_ErrorStreamCorruptStalled"},
  {OMX_ErrorStreamCorruptFatal, "OMX_ErrorStreamCorruptFatal"},
  {OMX_ErrorPortsNotConnected, "OMX_ErrorPortsNotConnected"},
  {OMX_ErrorContentURINotSpecified, "OMX_ErrorContentURINotSpecified"},
  {OMX_ErrorContentURIError, "OMX_ErrorContentURIError"},
  {OMX_ErrorCommandCanceled, "OMX_ErrorCommandCanceled"},
  {OMX_ErrorKhronosExtensions, "OMX_ErrorKhronosExtensions"},
  {OMX_ErrorVendorStartUnused, "OMX_ErrorVendorStartUnused"},
  {OMX_ErrorMax, "OMX_ErrorMax"}
};

typedef struct tiz_idx_str tiz_idx_str_t;
struct tiz_idx_str
{
  OMX_INDEXTYPE idx;
  OMX_STRING str;
};

static const tiz_idx_str_t tiz_idx_to_str_tbl[] = {
  {OMX_IndexComponentStartUnused, "OMX_IndexComponentStartUnused"},
  {OMX_IndexParamPriorityMgmt, "OMX_IndexParamPriorityMgmt"},
  {OMX_IndexParamAudioInit, "OMX_IndexParamAudioInit"},
  {OMX_IndexParamImageInit, "OMX_IndexParamImageInit"},
  {OMX_IndexParamVideoInit, "OMX_IndexParamVideoInit"},
  {OMX_IndexParamOtherInit, "OMX_IndexParamOtherInit"},
  {OMX_IndexParamNumAvailableStreams, "OMX_IndexParamNumAvailableStreams"},
  {OMX_IndexParamActiveStream, "OMX_IndexParamActiveStream"},
  {OMX_IndexParamSuspensionPolicy, "OMX_IndexParamSuspensionPolicy"},
  {OMX_IndexParamComponentSuspended, "OMX_IndexParamComponentSuspended"},
  {OMX_IndexConfigCapturing, "OMX_IndexConfigCapturing"},
  {OMX_IndexConfigCaptureMode, "OMX_IndexConfigCaptureMode"},
  {OMX_IndexAutoPauseAfterCapture, "OMX_IndexAutoPauseAfterCapture"},
  {OMX_IndexParamContentURI, "OMX_IndexParamContentURI"},
  {OMX_IndexReserved_0x0100000E, "OMX_IndexReserved_0x0100000E"},
  {OMX_IndexParamDisableResourceConcealment,
   "OMX_IndexParamDisableResourceConcealment"},
  {OMX_IndexConfigMetadataItemCount, "OMX_IndexConfigMetadataItemCount"},
  {OMX_IndexConfigContainerNodeCount, "OMX_IndexConfigContainerNodeCount"},
  {OMX_IndexConfigMetadataItem, "OMX_IndexConfigMetadataItem"},
  {OMX_IndexConfigCounterNodeID, "OMX_IndexConfigCounterNodeID"},
  {OMX_IndexParamMetadataFilterType, "OMX_IndexParamMetadataFilterType"},
  {OMX_IndexParamMetadataKeyFilter, "OMX_IndexParamMetadataKeyFilter"},
  {OMX_IndexConfigPriorityMgmt, "OMX_IndexConfigPriorityMgmt"},
  {OMX_IndexParamStandardComponentRole,
   "OMX_IndexParamStandardComponentRole"},
  {OMX_IndexConfigContentURI, "OMX_IndexConfigContentURI"},
  {OMX_IndexConfigCommonPortCapturing, "OMX_IndexConfigCommonPortCapturing"},
  {OMX_IndexConfigTunneledPortStatus, "OMX_IndexConfigTunneledPortStatus"},

  {OMX_IndexPortStartUnused, "OMX_IndexPortStartUnused"},
  {OMX_IndexParamPortDefinition, "OMX_IndexParamPortDefinition"},
  {OMX_IndexParamCompBufferSupplier, "OMX_IndexParamCompBufferSupplier"},
  {OMX_IndexReservedStartUnused, "OMX_IndexReservedStartUnused"},

    /* Audio parameters and configurations */
  {OMX_IndexAudioStartUnused, "OMX_IndexAudioStartUnused"},
  {OMX_IndexParamAudioPortFormat, "OMX_IndexParamAudioPortFormat"},
  {OMX_IndexParamAudioPcm, "OMX_IndexParamAudioPcm"},
  {OMX_IndexParamAudioAac, "OMX_IndexParamAudioAac"},
  {OMX_IndexParamAudioRa, "OMX_IndexParamAudioRa"},
  {OMX_IndexParamAudioMp3, "OMX_IndexParamAudioMp3"},
  {OMX_IndexParamAudioAdpcm, "OMX_IndexParamAudioAdpcm"},
  {OMX_IndexParamAudioG723, "OMX_IndexParamAudioG723"},
  {OMX_IndexParamAudioG729, "OMX_IndexParamAudioG729"},
  {OMX_IndexParamAudioAmr, "OMX_IndexParamAudioAmr"},
  {OMX_IndexParamAudioWma, "OMX_IndexParamAudioWma"},
  {OMX_IndexParamAudioSbc, "OMX_IndexParamAudioSbc"},
  {OMX_IndexParamAudioMidi, "OMX_IndexParamAudioMidi"},
  {OMX_IndexParamAudioGsm_FR, "OMX_IndexParamAudioGsm_FR"},
  {OMX_IndexParamAudioMidiLoadUserSound,
   "OMX_IndexParamAudioMidiLoadUserSound"},
  {OMX_IndexParamAudioG726, "OMX_IndexParamAudioG726"},
  {OMX_IndexParamAudioGsm_EFR, "OMX_IndexParamAudioGsm_EFR"},
  {OMX_IndexParamAudioGsm_HR, "OMX_IndexParamAudioGsm_HR"},
  {OMX_IndexParamAudioPdc_FR, "OMX_IndexParamAudioPdc_FR"},
  {OMX_IndexParamAudioPdc_EFR, "OMX_IndexParamAudioPdc_EFR"},
  {OMX_IndexParamAudioPdc_HR, "OMX_IndexParamAudioPdc_HR"},
  {OMX_IndexParamAudioTdma_FR, "OMX_IndexParamAudioTdma_FR"},
  {OMX_IndexParamAudioTdma_EFR, "OMX_IndexParamAudioTdma_EFR"},
  {OMX_IndexParamAudioQcelp8, "OMX_IndexParamAudioQcelp8"},
  {OMX_IndexParamAudioQcelp13, "OMX_IndexParamAudioQcelp13"},
  {OMX_IndexParamAudioEvrc, "OMX_IndexParamAudioEvrc"},
  {OMX_IndexParamAudioSmv, "OMX_IndexParamAudioSmv"},
  {OMX_IndexParamAudioVorbis, "OMX_IndexParamAudioVorbis"},
  {OMX_IndexConfigAudioMidiImmediateEvent,
   "OMX_IndexConfigAudioMidiImmediateEvent"},
  {OMX_IndexConfigAudioMidiControl, "OMX_IndexConfigAudioMidiControl"},
  {OMX_IndexConfigAudioMidiSoundBankProgram,
   "OMX_IndexConfigAudioMidiSoundBankProgram"},
  {OMX_IndexConfigAudioMidiStatus, "OMX_IndexConfigAudioMidiStatus"},
  {OMX_IndexConfigAudioMidiMetaEvent, "OMX_IndexConfigAudioMidiMetaEvent"},
  {OMX_IndexConfigAudioMidiMetaEventData,
   "OMX_IndexConfigAudioMidiMetaEventData"},
  {OMX_IndexConfigAudioVolume, "OMX_IndexConfigAudioVolume"},
  {OMX_IndexConfigAudioBalance, "OMX_IndexConfigAudioBalance"},
  {OMX_IndexConfigAudioChannelMute, "OMX_IndexConfigAudioChannelMute"},
  {OMX_IndexConfigAudioMute, "OMX_IndexConfigAudioMute"},

  {OMX_IndexConfigAudioLoudness, "OMX_IndexConfigAudioLoudness"},
  {OMX_IndexConfigAudioEchoCancelation,
   "OMX_IndexConfigAudioEchoCancelation"},
  {OMX_IndexConfigAudioNoiseReduction, "OMX_IndexConfigAudioNoiseReduction"},
  {OMX_IndexConfigAudioBass, "OMX_IndexConfigAudioBass"},
  {OMX_IndexConfigAudioTreble, "OMX_IndexConfigAudioTreble"},
  {OMX_IndexConfigAudioStereoWidening, "OMX_IndexConfigAudioStereoWidening"},
  {OMX_IndexConfigAudioChorus, "OMX_IndexConfigAudioChorus"},
  {OMX_IndexConfigAudioEqualizer, "OMX_IndexConfigAudioEqualizer"},
  {OMX_IndexConfigAudioReverberation, "OMX_IndexConfigAudioReverberation"},
  {OMX_IndexConfigAudioChannelVolume, "OMX_IndexConfigAudioChannelVolume"},
  {OMX_IndexConfigAudio3DOutput, "OMX_IndexConfigAudio3DOutput"},
  {OMX_IndexConfigAudio3DLocation, "OMX_IndexConfigAudio3DLocation"},
  {OMX_IndexParamAudio3DDopplerMode, "OMX_IndexParamAudio3DDopplerMode"},
  {OMX_IndexConfigAudio3DDopplerSettings, "OMX_IndexConfigAudio3DDopplerSettings"},
  {OMX_IndexConfigAudio3DLevels, "OMX_IndexConfigAudio3DLevels"},
  {OMX_IndexConfigAudio3DDistanceAttenuation, "OMX_IndexConfigAudio3DDistanceAttenuation"},
  {OMX_IndexConfigAudio3DDirectivitySettings, "OMX_IndexConfigAudio3DDirectivitySettings"},
  {OMX_IndexConfigAudio3DDirectivityOrientation, "OMX_IndexConfigAudio3DDirectivityOrientation"},
  {OMX_IndexConfigAudio3DMacroscopicOrientation, "OMX_IndexConfigAudio3DMacroscopicOrientation"},
  {OMX_IndexConfigAudio3DMacroscopicSize, "OMX_IndexConfigAudio3DMacroscopicSize"},
  {OMX_IndexParamAudioQueryChannelMapping, "OMX_IndexParamAudioQueryChannelMapping"},
  {OMX_IndexConfigAudioSbcBitpool, "OMX_IndexConfigAudioSbcBitpool"},
  {OMX_IndexConfigAudioAmrMode, "OMX_IndexConfigAudioAmrMode"},
  {OMX_IndexConfigAudioBitrate, "OMX_IndexConfigAudioBitrate"},
  {OMX_IndexConfigAudioAMRISFIndex, "OMX_IndexConfigAudioAMRISFIndex"},
  {OMX_IndexParamAudioFixedPoint, "OMX_IndexParamAudioFixedPoint"},

   /* Image specific parameters and configurations */
  {OMX_IndexImageStartUnused, "OMX_IndexImageStartUnused"},
  {OMX_IndexParamImagePortFormat, "OMX_IndexParamImagePortFormat"},
  {OMX_IndexParamFlashControl, "OMX_IndexParamFlashControl"},
  {OMX_IndexConfigFocusControl, "OMX_IndexConfigFocusControl"},
  {OMX_IndexParamQFactor, "OMX_IndexParamQFactor"},
  {OMX_IndexParamQuantizationTable, "OMX_IndexParamQuantizationTable"},
  {OMX_IndexParamHuffmanTable, "OMX_IndexParamHuffmanTable"},
  {OMX_IndexConfigFlashControl, "OMX_IndexConfigFlashControl"},
  {OMX_IndexConfigFlickerRejection, "OMX_IndexConfigFlickerRejection"},
  {OMX_IndexConfigImageHistogram, "OMX_IndexConfigImageHistogram"},
  {OMX_IndexConfigImageHistogramData, "OMX_IndexConfigImageHistogramData"},
  {OMX_IndexConfigImageHistogramInfo, "OMX_IndexConfigImageHistogramInfo"},
  {OMX_IndexConfigImageCaptureStarted, "OMX_IndexConfigImageCaptureStarted"},
  {OMX_IndexConfigImageCaptureEnded, "OMX_IndexConfigImageCaptureEnded"},
  
  /* Video specific parameters and configurations */
  {OMX_IndexVideoStartUnused, "OMX_IndexVideoStartUnused"},
  {OMX_IndexParamVideoPortFormat, "OMX_IndexParamVideoPortFormat"},
  {OMX_IndexParamVideoQuantization, "OMX_IndexParamVideoQuantization"},
  {OMX_IndexParamVideoFastUpdate, "OMX_IndexParamVideoFastUpdate"},
  {OMX_IndexParamVideoBitrate, "OMX_IndexParamVideoBitrate"},
  {OMX_IndexParamVideoMotionVector, "OMX_IndexParamVideoMotionVector"},
  {OMX_IndexParamVideoIntraRefresh, "OMX_IndexParamVideoIntraRefresh"},
  {OMX_IndexParamVideoErrorCorrection, "OMX_IndexParamVideoErrorCorrection"},
  {OMX_IndexParamVideoVBSMC, "OMX_IndexParamVideoVBSMC"},
  {OMX_IndexParamVideoMpeg2, "OMX_IndexParamVideoMpeg2"},
  {OMX_IndexParamVideoMpeg4, "OMX_IndexParamVideoMpeg4"},
  {OMX_IndexParamVideoWmv, "OMX_IndexParamVideoWmv"},
  {OMX_IndexParamVideoRv, "OMX_IndexParamVideoRv"},
  {OMX_IndexParamVideoAvc, "OMX_IndexParamVideoAvc"},
  {OMX_IndexParamVideoH263, "OMX_IndexParamVideoH263"},
  {OMX_IndexParamVideoProfileLevelQuerySupported,
   "OMX_IndexParamVideoProfileLevelQuerySupported"},
  {OMX_IndexParamVideoProfileLevelCurrent,
   "OMX_IndexParamVideoProfileLevelCurrent"},
  {OMX_IndexConfigVideoBitrate, "OMX_IndexConfigVideoBitrate"},
  {OMX_IndexConfigVideoFramerate, "OMX_IndexConfigVideoFramerate"},
  {OMX_IndexConfigVideoIntraVOPRefresh,
   "OMX_IndexConfigVideoIntraVOPRefresh"},
  {OMX_IndexConfigVideoIntraMBRefresh, "OMX_IndexConfigVideoIntraMBRefresh"},
  {OMX_IndexConfigVideoMBErrorReporting,
   "OMX_IndexConfigVideoMBErrorReporting"},
  {OMX_IndexParamVideoMacroblocksPerFrame,
   "OMX_IndexParamVideoMacroblocksPerFrame"},
  {OMX_IndexConfigVideoMacroBlockErrorMap,
   "OMX_IndexConfigVideoMacroBlockErrorMap"},
  {OMX_IndexParamVideoSliceFMO, "OMX_IndexParamVideoSliceFMO"},
  {OMX_IndexConfigVideoAVCIntraPeriod, "OMX_IndexConfigVideoAVCIntraPeriod"},
  {OMX_IndexConfigVideoNalSize, "OMX_IndexConfigVideoNalSize"},
  {OMX_IndexParamNalStreamFormatSupported, "OMX_IndexParamNalStreamFormatSupported"},
  {OMX_IndexParamNalStreamFormat, "OMX_IndexParamNalStreamFormat"},
  {OMX_IndexParamNalStreamFormatSelect, "OMX_IndexParamNalStreamFormatSelect"},
  {OMX_IndexParamVideoVC1, "OMX_IndexParamVideoVC1"},
  {OMX_IndexConfigVideoIntraPeriod, "OMX_IndexConfigVideoIntraPeriod"},
  {OMX_IndexConfigVideoIntraRefresh, "OMX_IndexConfigVideoIntraRefresh"},
  {OMX_IndexParamVideoVp8, "OMX_IndexParamVideoVp8"},
  {OMX_IndexConfigVideoVp8ReferenceFrame, "OMX_IndexConfigVideoVp8ReferenceFrame"},
  {OMX_IndexConfigVideoVp8ReferenceFrameType, "MX_IndexConfigVideoVp8ReferenceFrameType"},

    /* Image & Video common Configurations */
  {OMX_IndexCommonStartUnused, "OMX_IndexCommonStartUnused"},
  {OMX_IndexParamCommonDeblocking, "OMX_IndexParamCommonDeblocking"},
  {OMX_IndexParamCommonSensorMode, "OMX_IndexParamCommonSensorMode"},
  {OMX_IndexParamCommonInterleave, "OMX_IndexParamCommonInterleave"},
  {OMX_IndexConfigCommonColorFormatConversion,
   "OMX_IndexConfigCommonColorFormatConversion"},
  {OMX_IndexConfigCommonScale, "OMX_IndexConfigCommonScale"},
  {OMX_IndexConfigCommonImageFilter, "OMX_IndexConfigCommonImageFilter"},
  {OMX_IndexConfigCommonColorEnhancement,
   "OMX_IndexConfigCommonColorEnhancement"},
  {OMX_IndexConfigCommonColorKey, "OMX_IndexConfigCommonColorKey"},
  {OMX_IndexConfigCommonColorBlend, "OMX_IndexConfigCommonColorBlend"},
  {OMX_IndexConfigCommonFrameStabilisation,
   "OMX_IndexConfigCommonFrameStabilisation"},
  {OMX_IndexConfigCommonRotate, "OMX_IndexConfigCommonRotate"},
  {OMX_IndexConfigCommonMirror, "OMX_IndexConfigCommonMirror"},
  {OMX_IndexConfigCommonOutputPosition,
   "OMX_IndexConfigCommonOutputPosition"},
  {OMX_IndexConfigCommonInputCrop, "OMX_IndexConfigCommonInputCrop"},
  {OMX_IndexConfigCommonOutputCrop, "OMX_IndexConfigCommonOutputCrop"},
  {OMX_IndexConfigCommonDigitalZoom, "OMX_IndexConfigCommonDigitalZoom"},
  {OMX_IndexConfigCommonOpticalZoom, "OMX_IndexConfigCommonOpticalZoom"},
  {OMX_IndexConfigCommonWhiteBalance, "OMX_IndexConfigCommonWhiteBalance"},
  {OMX_IndexConfigCommonExposure, "OMX_IndexConfigCommonExposure"},

  {OMX_IndexConfigCommonContrast, "OMX_IndexConfigCommonContrast"},
  {OMX_IndexConfigCommonBrightness, "OMX_IndexConfigCommonBrightness"},
  {OMX_IndexConfigCommonBacklight, "OMX_IndexConfigCommonBacklight"},
  {OMX_IndexConfigCommonGamma, "OMX_IndexConfigCommonGamma"},
  {OMX_IndexConfigCommonSaturation, "OMX_IndexConfigCommonSaturation"},
  {OMX_IndexConfigCommonLightness, "OMX_IndexConfigCommonLightness"},
  {OMX_IndexConfigCommonExclusionRect, "OMX_IndexConfigCommonExclusionRect"},
  {OMX_IndexConfigCommonDithering, "OMX_IndexConfigCommonDithering"},
  {OMX_IndexConfigCommonPlaneBlend, "OMX_IndexConfigCommonPlaneBlend"},
  {OMX_IndexConfigCommonExposureValue, "OMX_IndexConfigCommonExposureValue"},
  {OMX_IndexConfigCommonOutputSize, "OMX_IndexConfigCommonOutputSize"},
  {OMX_IndexParamCommonExtraQuantData, "OMX_IndexParamCommonExtraQuantData"},
  {OMX_IndexReserved_0x0700002A, "OMX_IndexReserved_0x0700002A"},
  {OMX_IndexReserved_0x0700002B, "OMX_IndexReserved_0x0700002B"},
  {OMX_IndexConfigCommonTransitionEffect, "OMX_IndexConfigCommonTransitionEffect"},
  {OMX_IndexConfigSharpness, "OMX_IndexConfigSharpness"},
  {OMX_IndexConfigCommonExtDigitalZoom, "OMX_IndexConfigCommonExtDigitalZoom"},
  {OMX_IndexConfigCommonExtOpticalZoom, "OMX_IndexConfigCommonExtOpticalZoom"},
  {OMX_IndexConfigCommonCenterFieldOfView, "OMX_IndexConfigCommonCenterFieldOfView"},
  {OMX_IndexConfigImageExposureLock, "OMX_IndexConfigImageExposureLock"},
  {OMX_IndexConfigImageWhiteBalanceLock, "OMX_IndexConfigImageWhiteBalanceLock"},
  {OMX_IndexConfigImageFocusLock, "OMX_IndexConfigImageFocusLock"},
  {OMX_IndexConfigCommonFocusRange, "OMX_IndexConfigCommonFocusRange"},
  {OMX_IndexConfigImageFlashStatus, "OMX_IndexConfigImageFlashStatus"},
  {OMX_IndexConfigCommonExtCaptureMode, "OMX_IndexConfigCommonExtCaptureMode"},
  {OMX_IndexConfigCommonNDFilterControl, "OMX_IndexConfigCommonNDFilterControl"},
  {OMX_IndexConfigCommonAFAssistantLight, "OMX_IndexConfigCommonAFAssistantLight"},
  {OMX_IndexConfigCommonFocusRegionStatus, "OMX_IndexConfigCommonFocusRegionStatus"},
  {OMX_IndexConfigCommonFocusRegionControl, "OMX_IndexConfigCommonFocusRegionControl"},
  {OMX_IndexParamInterlaceFormat, "OMX_IndexParamInterlaceFormat"},
  {OMX_IndexConfigDeInterlace, "OMX_IndexConfigDeInterlace"},
  {OMX_IndexConfigStreamInterlaceFormats, "OMX_IndexConfigStreamInterlaceFormats"},

  /* Reserved Configuration range */
  {OMX_IndexOtherStartUnused, "OMX_IndexOtherStartUnused"},
  {OMX_IndexParamOtherPortFormat, "OMX_IndexParamOtherPortFormat"},
  {OMX_IndexConfigOtherPower, "OMX_IndexConfigOtherPower"},
  {OMX_IndexConfigOtherStats, "OMX_IndexConfigOtherStats"},

  /* Reserved Time range */
  {OMX_IndexTimeStartUnused, "OMX_IndexTimeStartUnused"},
  {OMX_IndexConfigTimeScale, "OMX_IndexConfigTimeScale"},
  {OMX_IndexConfigTimeClockState, "OMX_IndexConfigTimeClockState"},
  {OMX_IndexReserved_0x90000003, "OMX_IndexReserved_0x90000003"},
  {OMX_IndexConfigTimeCurrentMediaTime,
   "OMX_IndexConfigTimeCurrentMediaTime"},
  {OMX_IndexConfigTimeCurrentWallTime, "OMX_IndexConfigTimeCurrentWallTime"},
  {OMX_IndexReserved_0x09000006,
   "OMX_IndexReserved_0x09000006"},
  {OMX_IndexReserved_0x09000007,
   "OMX_IndexReserved_0x09000007"},
  {OMX_IndexConfigTimeMediaTimeRequest,
   "OMX_IndexConfigTimeMediaTimeRequest"},
  {OMX_IndexConfigTimeClientStartTime, "OMX_IndexConfigTimeClientStartTime"},
  {OMX_IndexConfigTimePosition, "OMX_IndexConfigTimePosition"},
  {OMX_IndexConfigTimeSeekMode, "OMX_IndexConfigTimeSeekMode"},
  {OMX_IndexConfigTimeCurrentReference, "OMX_IndexConfigTimeCurrentReference"},
  {OMX_IndexConfigTimeActiveRefClockUpdate, "OMX_IndexConfigTimeActiveRefClockUpdate"},
  {OMX_IndexConfigTimeRenderingDelay, "OMX_IndexConfigTimeRenderingDelay"},
  {OMX_IndexConfigTimeUpdate, "OMX_IndexConfigTimeUpdate"},

  /* Common or Domain Independent Time range */
  {OMX_IndexCommonIndependentStartUnused, "OMX_IndexCommonIndependentStartUnused"},
  {OMX_IndexConfigCommitMode, "OMX_IndexConfigCommitMode"},
  {OMX_IndexConfigCommit, "OMX_IndexConfigCommit"},
  {OMX_IndexConfigCallbackRequest, "OMX_IndexConfigCallbackRequest"},
  {OMX_IndexParamMediaContainer, "OMX_IndexParamMediaContainer"},
  {OMX_IndexParamReadOnlyBuffers, "OMX_IndexParamReadOnlyBuffers"},

  {OMX_IndexVendorStartUnused, "OMX_IndexVendorStartUnused"},
  {OMX_TizoniaIndexParamBufferPreAnnouncementsMode,
   "OMX_TizoniaIndexParamBufferPreAnnouncementsMode"},

  {OMX_IndexKhronosExtensions, "OMX_IndexKhronosExtensions"},
  {OMX_IndexVendorStartUnused, "OMX_IndexVendorStartUnused"},
  {OMX_IndexMax, "OMX_IndexMax"}
};

/*@observer@*/ const OMX_STRING
tiz_cmd_to_str (OMX_COMMANDTYPE a_cmd)
{
  const size_t count
    = sizeof (tiz_cmd_to_str_tbl) / sizeof (tiz_cmd_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_cmd_to_str_tbl[i].cmd == a_cmd)
        {
          return tiz_cmd_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL command";

}

/*@observer@*/ const OMX_STRING
tiz_state_to_str (OMX_STATETYPE a_id)
{
  const size_t count =
    sizeof (tiz_state_to_str_tbl) / sizeof (tiz_state_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_state_to_str_tbl[i].state == a_id)
        {
          return tiz_state_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL state";

}

/*@observer@*/ const OMX_STRING
tiz_evt_to_str (OMX_EVENTTYPE a_evt)
{
  const size_t count
    = sizeof (tiz_evt_to_str_tbl) / sizeof (tiz_evt_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_evt_to_str_tbl[i].evt == a_evt)
        {
          return tiz_evt_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL event";
}

/*@observer@*/ const OMX_STRING
tiz_err_to_str (OMX_ERRORTYPE a_err)
{
  const size_t count
    = sizeof (tiz_err_to_str_tbl) / sizeof (tiz_err_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_err_to_str_tbl[i].err == a_err)
        {
          return tiz_err_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL error";
}

/*@observer@*/ const OMX_STRING
tiz_dir_to_str (OMX_DIRTYPE a_dir)
{
  const size_t count
    = sizeof (tiz_dir_to_str_tbl) / sizeof (tiz_dir_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_dir_to_str_tbl[i].dir == a_dir)
        {
          return tiz_dir_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL port direction";
}

/*@observer@*/ const OMX_STRING
tiz_idx_to_str (OMX_INDEXTYPE a_idx)
{
  const size_t count
    = sizeof (tiz_idx_to_str_tbl) / sizeof (tiz_idx_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_idx_to_str_tbl[i].idx == a_idx)
        {
          return tiz_idx_to_str_tbl[i].str;
        }
    }

  return "Unknown OpenMAX IL index";
}
