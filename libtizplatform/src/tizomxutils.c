/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors and contributors
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
 * @file   tizomxutils.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - OpenMAX IL types utility functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <OMX_Core.h>
#include <OMX_TizoniaExt.h>

#include "tizplatform.h"

#include <assert.h>
#include <string.h>

typedef struct tiz_cmd_str tiz_cmd_str_t;
struct tiz_cmd_str
{
  OMX_COMMANDTYPE cmd;
  const OMX_STRING str;
};

static const tiz_cmd_str_t tiz_cmd_to_str_tbl[]
  = {{OMX_CommandStateSet, (const OMX_STRING) "OMX_CommandStateSet"},
     {OMX_CommandFlush, (const OMX_STRING) "OMX_CommandFlush"},
     {OMX_CommandPortDisable, (const OMX_STRING) "OMX_CommandPortDisable"},
     {OMX_CommandPortEnable, (const OMX_STRING) "OMX_CommandPortEnable"},
     {OMX_CommandMarkBuffer, (const OMX_STRING) "OMX_CommandMarkBuffer"},
     {OMX_CommandKhronosExtensions,
      (const OMX_STRING) "OMX_CommandKhronosExtensions"},
     {OMX_CommandVendorStartUnused,
      (const OMX_STRING) "OMX_CommandVendorStartUnused"},
     {OMX_CommandMax, (const OMX_STRING) "OMX_CommandMax"}};

typedef struct tiz_evt_str tiz_evt_str_t;
struct tiz_evt_str
{
  OMX_EVENTTYPE evt;
  const OMX_STRING str;
};

static const tiz_evt_str_t tiz_evt_to_str_tbl[] = {
  {OMX_EventCmdComplete, (const OMX_STRING) "OMX_EventCmdComplete"},
  {OMX_EventError, (const OMX_STRING) "OMX_EventError"},
  {OMX_EventMark, (const OMX_STRING) "OMX_EventMark"},
  {OMX_EventPortSettingsChanged,
   (const OMX_STRING) "OMX_EventPortSettingsChanged"},
  {OMX_EventBufferFlag, (const OMX_STRING) "OMX_EventBufferFlag"},
  {OMX_EventResourcesAcquired, (const OMX_STRING) "OMX_EventResourcesAcquired"},
  {OMX_EventComponentResumed, (const OMX_STRING) "OMX_EventComponentResumed"},
  {OMX_EventDynamicResourcesAvailable,
   (const OMX_STRING) "OMX_EventDynamicResourcesAvailable"},
  {OMX_EventPortFormatDetected,
   (const OMX_STRING) "OMX_EventPortFormatDetected"},
  {OMX_EventIndexSettingChanged,
   (const OMX_STRING) "OMX_EventIndexSettingChanged"},
  {OMX_EventPortNeedsDisable, (const OMX_STRING) "OMX_EventPortNeedsDisable"},
  {OMX_EventPortNeedsFlush, (const OMX_STRING) "OMX_EventPortNeedsFlush"},
  {OMX_EventKhronosExtensions, (const OMX_STRING) "OMX_EventKhronosExtensions"},
  {OMX_EventVendorStartUnused, (const OMX_STRING) "OMX_EventVendorStartUnused"},
  {OMX_EventMax, (const OMX_STRING) "OMX_EventMax"}};

typedef struct tiz_state_str tiz_state_str_t;

struct tiz_state_str
{
  OMX_STATETYPE state;
  const OMX_STRING str;
};

static const tiz_state_str_t tiz_state_to_str_tbl[] = {
  {OMX_StateReserved_0x00000000,
   (const OMX_STRING) "OMX_StateReserved_0x00000000"},
  {OMX_StateLoaded, (const OMX_STRING) "OMX_StateLoaded"},
  {OMX_StateIdle, (const OMX_STRING) "OMX_StateIdle"},
  {OMX_StateExecuting, (const OMX_STRING) "OMX_StateExecuting"},
  {OMX_StatePause, (const OMX_STRING) "OMX_StatePause"},
  {OMX_StateWaitForResources, (const OMX_STRING) "OMX_StateWaitForResources"},
  {OMX_StateKhronosExtensions, (const OMX_STRING) "OMX_StateKhronosExtensions"},
  {OMX_StateVendorStartUnused, (const OMX_STRING) "OMX_StateVendorStartUnused"},
  {OMX_StateMax, (const OMX_STRING) "OMX_StateMax"}};

typedef struct tiz_dir_str tiz_dir_str_t;
struct tiz_dir_str
{
  OMX_DIRTYPE dir;
  const OMX_STRING str;
};

static const tiz_dir_str_t tiz_dir_to_str_tbl[]
  = {{OMX_DirInput, (const OMX_STRING) "OMX_DirInput"},
     {OMX_DirOutput, (const OMX_STRING) "OMX_DirOutput"},
     {OMX_DirMax, (const OMX_STRING) "OMX_DirMax"}};

typedef struct tiz_domain_str tiz_domain_str_t;
struct tiz_domain_str
{
  OMX_PORTDOMAINTYPE domain;
  const OMX_STRING str;
};

static const tiz_domain_str_t tiz_domain_to_str_tbl[]
  = {{OMX_PortDomainAudio, (const OMX_STRING) "OMX_PortDomainAudio"},
     {OMX_PortDomainVideo, (const OMX_STRING) "OMX_PortDomainVideo"},
     {OMX_PortDomainImage, (const OMX_STRING) "OMX_PortDomainImage"},
     {OMX_PortDomainOther, (const OMX_STRING) "OMX_PortDomainOther"},
     {OMX_PortDomainKhronosExtensions,
      (const OMX_STRING) "OMX_PortDomainKhronosExtensions"},
     {OMX_PortDomainVendorStartUnused,
      (const OMX_STRING) "OMX_PortDomainVendorStartUnused"},
     {OMX_PortDomainMax, (const OMX_STRING) "OMX_PortDomainMax"}};

typedef struct tiz_err_str tiz_err_str_t;
struct tiz_err_str
{
  OMX_ERRORTYPE err;
  const OMX_STRING str;
};

static const tiz_err_str_t tiz_err_to_str_tbl[] = {
  {OMX_ErrorNone, (const OMX_STRING) "OMX_ErrorNone"},
  {OMX_ErrorInsufficientResources,
   (const OMX_STRING) "OMX_ErrorInsufficientResources"},
  {OMX_ErrorUndefined, (const OMX_STRING) "OMX_ErrorUndefined"},
  {OMX_ErrorInvalidComponentName,
   (const OMX_STRING) "OMX_ErrorInvalidComponentName"},
  {OMX_ErrorComponentNotFound, (const OMX_STRING) "OMX_ErrorComponentNotFound"},
  {OMX_ErrorReserved_0x80001004,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001004"},
  {OMX_ErrorBadParameter, (const OMX_STRING) "OMX_ErrorBadParameter"},
  {OMX_ErrorNotImplemented, (const OMX_STRING) "OMX_ErrorNotImplemented"},
  {OMX_ErrorUnderflow, (const OMX_STRING) "OMX_ErrorUnderflow"},
  {OMX_ErrorOverflow, (const OMX_STRING) "OMX_ErrorOverflow"},
  {OMX_ErrorHardware, (const OMX_STRING) "OMX_ErrorHardware"},
  {OMX_ErrorReserved_0x8000100A,
   (const OMX_STRING) "OMX_ErrorReserved_0x8000100A"},
  {OMX_ErrorStreamCorrupt, (const OMX_STRING) "OMX_ErrorStreamCorrupt"},
  {OMX_ErrorPortsNotCompatible,
   (const OMX_STRING) "OMX_ErrorPortsNotCompatible"},
  {OMX_ErrorResourcesLost, (const OMX_STRING) "OMX_ErrorResourcesLost"},
  {OMX_ErrorNoMore, (const OMX_STRING) "OMX_ErrorNoMore"},
  {OMX_ErrorVersionMismatch, (const OMX_STRING) "OMX_ErrorVersionMismatch"},
  {OMX_ErrorNotReady, (const OMX_STRING) "OMX_ErrorNotReady"},
  {OMX_ErrorTimeout, (const OMX_STRING) "OMX_ErrorTimeout"},
  {OMX_ErrorSameState, (const OMX_STRING) "OMX_ErrorSameState"},
  {OMX_ErrorResourcesPreempted,
   (const OMX_STRING) "OMX_ErrorResourcesPreempted"},
  {OMX_ErrorReserved_0x80001014,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001014"},
  {OMX_ErrorReserved_0x80001015,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001015"},
  {OMX_ErrorReserved_0x80001016,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001016"},
  {OMX_ErrorIncorrectStateTransition,
   (const OMX_STRING) "OMX_ErrorIncorrectStateTransition"},
  {OMX_ErrorIncorrectStateOperation,
   (const OMX_STRING) "OMX_ErrorIncorrectStateOperation"},
  {OMX_ErrorUnsupportedSetting,
   (const OMX_STRING) "OMX_ErrorUnsupportedSetting"},
  {OMX_ErrorUnsupportedIndex, (const OMX_STRING) "OMX_ErrorUnsupportedIndex"},
  {OMX_ErrorBadPortIndex, (const OMX_STRING) "OMX_ErrorBadPortIndex"},
  {OMX_ErrorPortUnpopulated, (const OMX_STRING) "OMX_ErrorPortUnpopulated"},
  {OMX_ErrorComponentSuspended,
   (const OMX_STRING) "OMX_ErrorComponentSuspended"},
  {OMX_ErrorDynamicResourcesUnavailable,
   (const OMX_STRING) "OMX_ErrorDynamicResourcesUnavailable"},
  {OMX_ErrorMbErrorsInFrame, (const OMX_STRING) "OMX_ErrorMbErrorsInFrame"},
  {OMX_ErrorFormatNotDetected, (const OMX_STRING) "OMX_ErrorFormatNotDetected"},
  {OMX_ErrorReserved_0x80001021,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001021"},
  {OMX_ErrorReserved_0x80001022,
   (const OMX_STRING) "OMX_ErrorReserved_0x80001022"},
  {OMX_ErrorSeperateTablesUsed,
   (const OMX_STRING) "OMX_ErrorSeperateTablesUsed"},
  {OMX_ErrorTunnelingUnsupported,
   (const OMX_STRING) "OMX_ErrorTunnelingUnsupported"},
  {OMX_ErrorInvalidMode, (const OMX_STRING) "OMX_ErrorInvalidMode"},
  {OMX_ErrorStreamCorruptStalled,
   (const OMX_STRING) "OMX_ErrorStreamCorruptStalled"},
  {OMX_ErrorStreamCorruptFatal,
   (const OMX_STRING) "OMX_ErrorStreamCorruptFatal"},
  {OMX_ErrorPortsNotConnected, (const OMX_STRING) "OMX_ErrorPortsNotConnected"},
  {OMX_ErrorContentURINotSpecified,
   (const OMX_STRING) "OMX_ErrorContentURINotSpecified"},
  {OMX_ErrorContentURIError, (const OMX_STRING) "OMX_ErrorContentURIError"},
  {OMX_ErrorCommandCanceled, (const OMX_STRING) "OMX_ErrorCommandCanceled"},
  {OMX_ErrorKhronosExtensions, (const OMX_STRING) "OMX_ErrorKhronosExtensions"},
  {OMX_ErrorVendorStartUnused, (const OMX_STRING) "OMX_ErrorVendorStartUnused"},
  {OMX_ErrorMax, (const OMX_STRING) "OMX_ErrorMax"}};

typedef struct tiz_idx_str tiz_idx_str_t;
struct tiz_idx_str
{
  OMX_INDEXTYPE idx;
  const OMX_STRING str;
};

static const tiz_idx_str_t tiz_idx_to_str_tbl[] = {
  {OMX_IndexComponentStartUnused,
   (const OMX_STRING) "OMX_IndexComponentStartUnused"},
  {OMX_IndexParamPriorityMgmt, (const OMX_STRING) "OMX_IndexParamPriorityMgmt"},
  {OMX_IndexParamAudioInit, (const OMX_STRING) "OMX_IndexParamAudioInit"},
  {OMX_IndexParamImageInit, (const OMX_STRING) "OMX_IndexParamImageInit"},
  {OMX_IndexParamVideoInit, (const OMX_STRING) "OMX_IndexParamVideoInit"},
  {OMX_IndexParamOtherInit, (const OMX_STRING) "OMX_IndexParamOtherInit"},
  {OMX_IndexParamNumAvailableStreams,
   (const OMX_STRING) "OMX_IndexParamNumAvailableStreams"},
  {OMX_IndexParamActiveStream, (const OMX_STRING) "OMX_IndexParamActiveStream"},
  {OMX_IndexParamSuspensionPolicy,
   (const OMX_STRING) "OMX_IndexParamSuspensionPolicy"},
  {OMX_IndexParamComponentSuspended,
   (const OMX_STRING) "OMX_IndexParamComponentSuspended"},
  {OMX_IndexConfigCapturing, (const OMX_STRING) "OMX_IndexConfigCapturing"},
  {OMX_IndexConfigCaptureMode, (const OMX_STRING) "OMX_IndexConfigCaptureMode"},
  {OMX_IndexAutoPauseAfterCapture,
   (const OMX_STRING) "OMX_IndexAutoPauseAfterCapture"},
  {OMX_IndexParamContentURI, (const OMX_STRING) "OMX_IndexParamContentURI"},
  {OMX_IndexReserved_0x0100000E,
   (const OMX_STRING) "OMX_IndexReserved_0x0100000E"},
  {OMX_IndexParamDisableResourceConcealment,
   (const OMX_STRING) "OMX_IndexParamDisableResourceConcealment"},
  {OMX_IndexConfigMetadataItemCount,
   (const OMX_STRING) "OMX_IndexConfigMetadataItemCount"},
  {OMX_IndexConfigContainerNodeCount,
   (const OMX_STRING) "OMX_IndexConfigContainerNodeCount"},
  {OMX_IndexConfigMetadataItem,
   (const OMX_STRING) "OMX_IndexConfigMetadataItem"},
  {OMX_IndexConfigCounterNodeID,
   (const OMX_STRING) "OMX_IndexConfigCounterNodeID"},
  {OMX_IndexParamMetadataFilterType,
   (const OMX_STRING) "OMX_IndexParamMetadataFilterType"},
  {OMX_IndexParamMetadataKeyFilter,
   (const OMX_STRING) "OMX_IndexParamMetadataKeyFilter"},
  {OMX_IndexConfigPriorityMgmt,
   (const OMX_STRING) "OMX_IndexConfigPriorityMgmt"},
  {OMX_IndexParamStandardComponentRole,
   (const OMX_STRING) "OMX_IndexParamStandardComponentRole"},
  {OMX_IndexConfigContentURI, (const OMX_STRING) "OMX_IndexConfigContentURI"},
  {OMX_IndexConfigCommonPortCapturing,
   (const OMX_STRING) "OMX_IndexConfigCommonPortCapturing"},
  {OMX_IndexConfigTunneledPortStatus,
   (const OMX_STRING) "OMX_IndexConfigTunneledPortStatus"},
  {OMX_IndexPortStartUnused, (const OMX_STRING) "OMX_IndexPortStartUnused"},
  {OMX_IndexParamPortDefinition,
   (const OMX_STRING) "OMX_IndexParamPortDefinition"},
  {OMX_IndexParamCompBufferSupplier,
   (const OMX_STRING) "OMX_IndexParamCompBufferSupplier"},
  {OMX_IndexReservedStartUnused,
   (const OMX_STRING) "OMX_IndexReservedStartUnused"},

  /* Audio parameters and configurations */
  {OMX_IndexAudioStartUnused, (const OMX_STRING) "OMX_IndexAudioStartUnused"},
  {OMX_IndexParamAudioPortFormat,
   (const OMX_STRING) "OMX_IndexParamAudioPortFormat"},
  {OMX_IndexParamAudioPcm, (const OMX_STRING) "OMX_IndexParamAudioPcm"},
  {OMX_IndexParamAudioAac, (const OMX_STRING) "OMX_IndexParamAudioAac"},
  {OMX_IndexParamAudioRa, (const OMX_STRING) "OMX_IndexParamAudioRa"},
  {OMX_IndexParamAudioMp3, (const OMX_STRING) "OMX_IndexParamAudioMp3"},
  {OMX_IndexParamAudioAdpcm, (const OMX_STRING) "OMX_IndexParamAudioAdpcm"},
  {OMX_IndexParamAudioG723, (const OMX_STRING) "OMX_IndexParamAudioG723"},
  {OMX_IndexParamAudioG729, (const OMX_STRING) "OMX_IndexParamAudioG729"},
  {OMX_IndexParamAudioAmr, (const OMX_STRING) "OMX_IndexParamAudioAmr"},
  {OMX_IndexParamAudioWma, (const OMX_STRING) "OMX_IndexParamAudioWma"},
  {OMX_IndexParamAudioSbc, (const OMX_STRING) "OMX_IndexParamAudioSbc"},
  {OMX_IndexParamAudioMidi, (const OMX_STRING) "OMX_IndexParamAudioMidi"},
  {OMX_IndexParamAudioGsm_FR, (const OMX_STRING) "OMX_IndexParamAudioGsm_FR"},
  {OMX_IndexParamAudioMidiLoadUserSound,
   (const OMX_STRING) "OMX_IndexParamAudioMidiLoadUserSound"},
  {OMX_IndexParamAudioG726, (const OMX_STRING) "OMX_IndexParamAudioG726"},
  {OMX_IndexParamAudioGsm_EFR, (const OMX_STRING) "OMX_IndexParamAudioGsm_EFR"},
  {OMX_IndexParamAudioGsm_HR, (const OMX_STRING) "OMX_IndexParamAudioGsm_HR"},
  {OMX_IndexParamAudioPdc_FR, (const OMX_STRING) "OMX_IndexParamAudioPdc_FR"},
  {OMX_IndexParamAudioPdc_EFR, (const OMX_STRING) "OMX_IndexParamAudioPdc_EFR"},
  {OMX_IndexParamAudioPdc_HR, (const OMX_STRING) "OMX_IndexParamAudioPdc_HR"},
  {OMX_IndexParamAudioTdma_FR, (const OMX_STRING) "OMX_IndexParamAudioTdma_FR"},
  {OMX_IndexParamAudioTdma_EFR,
   (const OMX_STRING) "OMX_IndexParamAudioTdma_EFR"},
  {OMX_IndexParamAudioQcelp8, (const OMX_STRING) "OMX_IndexParamAudioQcelp8"},
  {OMX_IndexParamAudioQcelp13, (const OMX_STRING) "OMX_IndexParamAudioQcelp13"},
  {OMX_IndexParamAudioEvrc, (const OMX_STRING) "OMX_IndexParamAudioEvrc"},
  {OMX_IndexParamAudioSmv, (const OMX_STRING) "OMX_IndexParamAudioSmv"},
  {OMX_IndexParamAudioVorbis, (const OMX_STRING) "OMX_IndexParamAudioVorbis"},
  {OMX_IndexConfigAudioMidiImmediateEvent,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiImmediateEvent"},
  {OMX_IndexConfigAudioMidiControl,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiControl"},
  {OMX_IndexConfigAudioMidiSoundBankProgram,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiSoundBankProgram"},
  {OMX_IndexConfigAudioMidiStatus,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiStatus"},
  {OMX_IndexConfigAudioMidiMetaEvent,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiMetaEvent"},
  {OMX_IndexConfigAudioMidiMetaEventData,
   (const OMX_STRING) "OMX_IndexConfigAudioMidiMetaEventData"},
  {OMX_IndexConfigAudioVolume, (const OMX_STRING) "OMX_IndexConfigAudioVolume"},
  {OMX_IndexConfigAudioBalance,
   (const OMX_STRING) "OMX_IndexConfigAudioBalance"},
  {OMX_IndexConfigAudioChannelMute,
   (const OMX_STRING) "OMX_IndexConfigAudioChannelMute"},
  {OMX_IndexConfigAudioMute, (const OMX_STRING) "OMX_IndexConfigAudioMute"},
  {OMX_IndexConfigAudioLoudness,
   (const OMX_STRING) "OMX_IndexConfigAudioLoudness"},
  {OMX_IndexConfigAudioEchoCancelation,
   (const OMX_STRING) "OMX_IndexConfigAudioEchoCancelation"},
  {OMX_IndexConfigAudioNoiseReduction,
   (const OMX_STRING) "OMX_IndexConfigAudioNoiseReduction"},
  {OMX_IndexConfigAudioBass, (const OMX_STRING) "OMX_IndexConfigAudioBass"},
  {OMX_IndexConfigAudioTreble, (const OMX_STRING) "OMX_IndexConfigAudioTreble"},
  {OMX_IndexConfigAudioStereoWidening,
   (const OMX_STRING) "OMX_IndexConfigAudioStereoWidening"},
  {OMX_IndexConfigAudioChorus, (const OMX_STRING) "OMX_IndexConfigAudioChorus"},
  {OMX_IndexConfigAudioEqualizer,
   (const OMX_STRING) "OMX_IndexConfigAudioEqualizer"},
  {OMX_IndexConfigAudioReverberation,
   (const OMX_STRING) "OMX_IndexConfigAudioReverberation"},
  {OMX_IndexConfigAudioChannelVolume,
   (const OMX_STRING) "OMX_IndexConfigAudioChannelVolume"},
  {OMX_IndexConfigAudio3DOutput,
   (const OMX_STRING) "OMX_IndexConfigAudio3DOutput"},
  {OMX_IndexConfigAudio3DLocation,
   (const OMX_STRING) "OMX_IndexConfigAudio3DLocation"},
  {OMX_IndexParamAudio3DDopplerMode,
   (const OMX_STRING) "OMX_IndexParamAudio3DDopplerMode"},
  {OMX_IndexConfigAudio3DDopplerSettings,
   (const OMX_STRING) "OMX_IndexConfigAudio3DDopplerSettings"},
  {OMX_IndexConfigAudio3DLevels,
   (const OMX_STRING) "OMX_IndexConfigAudio3DLevels"},
  {OMX_IndexConfigAudio3DDistanceAttenuation,
   (const OMX_STRING) "OMX_IndexConfigAudio3DDistanceAttenuation"},
  {OMX_IndexConfigAudio3DDirectivitySettings,
   (const OMX_STRING) "OMX_IndexConfigAudio3DDirectivitySettings"},
  {OMX_IndexConfigAudio3DDirectivityOrientation,
   (const OMX_STRING) "OMX_IndexConfigAudio3DDirectivityOrientation"},
  {OMX_IndexConfigAudio3DMacroscopicOrientation,
   (const OMX_STRING) "OMX_IndexConfigAudio3DMacroscopicOrientation"},
  {OMX_IndexConfigAudio3DMacroscopicSize,
   (const OMX_STRING) "OMX_IndexConfigAudio3DMacroscopicSize"},
  {OMX_IndexParamAudioQueryChannelMapping,
   (const OMX_STRING) "OMX_IndexParamAudioQueryChannelMapping"},
  {OMX_IndexConfigAudioSbcBitpool,
   (const OMX_STRING) "OMX_IndexConfigAudioSbcBitpool"},
  {OMX_IndexConfigAudioAmrMode,
   (const OMX_STRING) "OMX_IndexConfigAudioAmrMode"},
  {OMX_IndexConfigAudioBitrate,
   (const OMX_STRING) "OMX_IndexConfigAudioBitrate"},
  {OMX_IndexConfigAudioAMRISFIndex,
   (const OMX_STRING) "OMX_IndexConfigAudioAMRISFIndex"},
  {OMX_IndexParamAudioFixedPoint,
   (const OMX_STRING) "OMX_IndexParamAudioFixedPoint"},

  /* Image specific parameters and configurations */
  {OMX_IndexImageStartUnused, (const OMX_STRING) "OMX_IndexImageStartUnused"},
  {OMX_IndexParamImagePortFormat,
   (const OMX_STRING) "OMX_IndexParamImagePortFormat"},
  {OMX_IndexParamFlashControl, (const OMX_STRING) "OMX_IndexParamFlashControl"},
  {OMX_IndexConfigFocusControl,
   (const OMX_STRING) "OMX_IndexConfigFocusControl"},
  {OMX_IndexParamQFactor, (const OMX_STRING) "OMX_IndexParamQFactor"},
  {OMX_IndexParamQuantizationTable,
   (const OMX_STRING) "OMX_IndexParamQuantizationTable"},
  {OMX_IndexParamHuffmanTable, (const OMX_STRING) "OMX_IndexParamHuffmanTable"},
  {OMX_IndexConfigFlashControl,
   (const OMX_STRING) "OMX_IndexConfigFlashControl"},
  {OMX_IndexConfigFlickerRejection,
   (const OMX_STRING) "OMX_IndexConfigFlickerRejection"},
  {OMX_IndexConfigImageHistogram,
   (const OMX_STRING) "OMX_IndexConfigImageHistogram"},
  {OMX_IndexConfigImageHistogramData,
   (const OMX_STRING) "OMX_IndexConfigImageHistogramData"},
  {OMX_IndexConfigImageHistogramInfo,
   (const OMX_STRING) "OMX_IndexConfigImageHistogramInfo"},
  {OMX_IndexConfigImageCaptureStarted,
   (const OMX_STRING) "OMX_IndexConfigImageCaptureStarted"},
  {OMX_IndexConfigImageCaptureEnded,
   (const OMX_STRING) "OMX_IndexConfigImageCaptureEnded"},

  /* Video specific parameters and configurations */
  {OMX_IndexVideoStartUnused, (const OMX_STRING) "OMX_IndexVideoStartUnused"},
  {OMX_IndexParamVideoPortFormat,
   (const OMX_STRING) "OMX_IndexParamVideoPortFormat"},
  {OMX_IndexParamVideoQuantization,
   (const OMX_STRING) "OMX_IndexParamVideoQuantization"},
  {OMX_IndexParamVideoFastUpdate,
   (const OMX_STRING) "OMX_IndexParamVideoFastUpdate"},
  {OMX_IndexParamVideoBitrate, (const OMX_STRING) "OMX_IndexParamVideoBitrate"},
  {OMX_IndexParamVideoMotionVector,
   (const OMX_STRING) "OMX_IndexParamVideoMotionVector"},
  {OMX_IndexParamVideoIntraRefresh,
   (const OMX_STRING) "OMX_IndexParamVideoIntraRefresh"},
  {OMX_IndexParamVideoErrorCorrection,
   (const OMX_STRING) "OMX_IndexParamVideoErrorCorrection"},
  {OMX_IndexParamVideoVBSMC, (const OMX_STRING) "OMX_IndexParamVideoVBSMC"},
  {OMX_IndexParamVideoMpeg2, (const OMX_STRING) "OMX_IndexParamVideoMpeg2"},
  {OMX_IndexParamVideoMpeg4, (const OMX_STRING) "OMX_IndexParamVideoMpeg4"},
  {OMX_IndexParamVideoWmv, (const OMX_STRING) "OMX_IndexParamVideoWmv"},
  {OMX_IndexParamVideoRv, (const OMX_STRING) "OMX_IndexParamVideoRv"},
  {OMX_IndexParamVideoAvc, (const OMX_STRING) "OMX_IndexParamVideoAvc"},
  {OMX_IndexParamVideoH263, (const OMX_STRING) "OMX_IndexParamVideoH263"},
  {OMX_IndexParamVideoProfileLevelQuerySupported,
   (const OMX_STRING) "OMX_IndexParamVideoProfileLevelQuerySupported"},
  {OMX_IndexParamVideoProfileLevelCurrent,
   (const OMX_STRING) "OMX_IndexParamVideoProfileLevelCurrent"},
  {OMX_IndexConfigVideoBitrate,
   (const OMX_STRING) "OMX_IndexConfigVideoBitrate"},
  {OMX_IndexConfigVideoFramerate,
   (const OMX_STRING) "OMX_IndexConfigVideoFramerate"},
  {OMX_IndexConfigVideoIntraVOPRefresh,
   (const OMX_STRING) "OMX_IndexConfigVideoIntraVOPRefresh"},
  {OMX_IndexConfigVideoIntraMBRefresh,
   (const OMX_STRING) "OMX_IndexConfigVideoIntraMBRefresh"},
  {OMX_IndexConfigVideoMBErrorReporting,
   (const OMX_STRING) "OMX_IndexConfigVideoMBErrorReporting"},
  {OMX_IndexParamVideoMacroblocksPerFrame,
   (const OMX_STRING) "OMX_IndexParamVideoMacroblocksPerFrame"},
  {OMX_IndexConfigVideoMacroBlockErrorMap,
   (const OMX_STRING) "OMX_IndexConfigVideoMacroBlockErrorMap"},
  {OMX_IndexParamVideoSliceFMO,
   (const OMX_STRING) "OMX_IndexParamVideoSliceFMO"},
  {OMX_IndexConfigVideoAVCIntraPeriod,
   (const OMX_STRING) "OMX_IndexConfigVideoAVCIntraPeriod"},
  {OMX_IndexConfigVideoNalSize,
   (const OMX_STRING) "OMX_IndexConfigVideoNalSize"},
  {OMX_IndexParamNalStreamFormatSupported,
   (const OMX_STRING) "OMX_IndexParamNalStreamFormatSupported"},
  {OMX_IndexParamNalStreamFormat,
   (const OMX_STRING) "OMX_IndexParamNalStreamFormat"},
  {OMX_IndexParamNalStreamFormatSelect,
   (const OMX_STRING) "OMX_IndexParamNalStreamFormatSelect"},
  {OMX_IndexParamVideoVC1, (const OMX_STRING) "OMX_IndexParamVideoVC1"},
  {OMX_IndexConfigVideoIntraPeriod,
   (const OMX_STRING) "OMX_IndexConfigVideoIntraPeriod"},
  {OMX_IndexConfigVideoIntraRefresh,
   (const OMX_STRING) "OMX_IndexConfigVideoIntraRefresh"},
  {OMX_IndexParamVideoVp8, (const OMX_STRING) "OMX_IndexParamVideoVp8"},
  {OMX_IndexConfigVideoVp8ReferenceFrame,
   (const OMX_STRING) "OMX_IndexConfigVideoVp8ReferenceFrame"},
  {OMX_IndexConfigVideoVp8ReferenceFrameType,
   (const OMX_STRING) "OMX_IndexConfigVideoVp8ReferenceFrameType"},

  /* Image & Video common Configurations */
  {OMX_IndexCommonStartUnused, (const OMX_STRING) "OMX_IndexCommonStartUnused"},
  {OMX_IndexParamCommonDeblocking,
   (const OMX_STRING) "OMX_IndexParamCommonDeblocking"},
  {OMX_IndexParamCommonSensorMode,
   (const OMX_STRING) "OMX_IndexParamCommonSensorMode"},
  {OMX_IndexParamCommonInterleave,
   (const OMX_STRING) "OMX_IndexParamCommonInterleave"},
  {OMX_IndexConfigCommonColorFormatConversion,
   (const OMX_STRING) "OMX_IndexConfigCommonColorFormatConversion"},
  {OMX_IndexConfigCommonScale, (const OMX_STRING) "OMX_IndexConfigCommonScale"},
  {OMX_IndexConfigCommonImageFilter,
   (const OMX_STRING) "OMX_IndexConfigCommonImageFilter"},
  {OMX_IndexConfigCommonColorEnhancement,
   (const OMX_STRING) "OMX_IndexConfigCommonColorEnhancement"},
  {OMX_IndexConfigCommonColorKey,
   (const OMX_STRING) "OMX_IndexConfigCommonColorKey"},
  {OMX_IndexConfigCommonColorBlend,
   (const OMX_STRING) "OMX_IndexConfigCommonColorBlend"},
  {OMX_IndexConfigCommonFrameStabilisation,
   (const OMX_STRING) "OMX_IndexConfigCommonFrameStabilisation"},
  {OMX_IndexConfigCommonRotate,
   (const OMX_STRING) "OMX_IndexConfigCommonRotate"},
  {OMX_IndexConfigCommonMirror,
   (const OMX_STRING) "OMX_IndexConfigCommonMirror"},
  {OMX_IndexConfigCommonOutputPosition,
   (const OMX_STRING) "OMX_IndexConfigCommonOutputPosition"},
  {OMX_IndexConfigCommonInputCrop,
   (const OMX_STRING) "OMX_IndexConfigCommonInputCrop"},
  {OMX_IndexConfigCommonOutputCrop,
   (const OMX_STRING) "OMX_IndexConfigCommonOutputCrop"},
  {OMX_IndexConfigCommonDigitalZoom,
   (const OMX_STRING) "OMX_IndexConfigCommonDigitalZoom"},
  {OMX_IndexConfigCommonOpticalZoom,
   (const OMX_STRING) "OMX_IndexConfigCommonOpticalZoom"},
  {OMX_IndexConfigCommonWhiteBalance,
   (const OMX_STRING) "OMX_IndexConfigCommonWhiteBalance"},
  {OMX_IndexConfigCommonExposure,
   (const OMX_STRING) "OMX_IndexConfigCommonExposure"},
  {OMX_IndexConfigCommonContrast,
   (const OMX_STRING) "OMX_IndexConfigCommonContrast"},
  {OMX_IndexConfigCommonBrightness,
   (const OMX_STRING) "OMX_IndexConfigCommonBrightness"},
  {OMX_IndexConfigCommonBacklight,
   (const OMX_STRING) "OMX_IndexConfigCommonBacklight"},
  {OMX_IndexConfigCommonGamma, (const OMX_STRING) "OMX_IndexConfigCommonGamma"},
  {OMX_IndexConfigCommonSaturation,
   (const OMX_STRING) "OMX_IndexConfigCommonSaturation"},
  {OMX_IndexConfigCommonLightness,
   (const OMX_STRING) "OMX_IndexConfigCommonLightness"},
  {OMX_IndexConfigCommonExclusionRect,
   (const OMX_STRING) "OMX_IndexConfigCommonExclusionRect"},
  {OMX_IndexConfigCommonDithering,
   (const OMX_STRING) "OMX_IndexConfigCommonDithering"},
  {OMX_IndexConfigCommonPlaneBlend,
   (const OMX_STRING) "OMX_IndexConfigCommonPlaneBlend"},
  {OMX_IndexConfigCommonExposureValue,
   (const OMX_STRING) "OMX_IndexConfigCommonExposureValue"},
  {OMX_IndexConfigCommonOutputSize,
   (const OMX_STRING) "OMX_IndexConfigCommonOutputSize"},
  {OMX_IndexParamCommonExtraQuantData,
   (const OMX_STRING) "OMX_IndexParamCommonExtraQuantData"},
  {OMX_IndexReserved_0x0700002A,
   (const OMX_STRING) "OMX_IndexReserved_0x0700002A"},
  {OMX_IndexReserved_0x0700002B,
   (const OMX_STRING) "OMX_IndexReserved_0x0700002B"},
  {OMX_IndexConfigCommonTransitionEffect,
   (const OMX_STRING) "OMX_IndexConfigCommonTransitionEffect"},
  {OMX_IndexConfigSharpness, (const OMX_STRING) "OMX_IndexConfigSharpness"},
  {OMX_IndexConfigCommonExtDigitalZoom,
   (const OMX_STRING) "OMX_IndexConfigCommonExtDigitalZoom"},
  {OMX_IndexConfigCommonExtOpticalZoom,
   (const OMX_STRING) "OMX_IndexConfigCommonExtOpticalZoom"},
  {OMX_IndexConfigCommonCenterFieldOfView,
   (const OMX_STRING) "OMX_IndexConfigCommonCenterFieldOfView"},
  {OMX_IndexConfigImageExposureLock,
   (const OMX_STRING) "OMX_IndexConfigImageExposureLock"},
  {OMX_IndexConfigImageWhiteBalanceLock,
   (const OMX_STRING) "OMX_IndexConfigImageWhiteBalanceLock"},
  {OMX_IndexConfigImageFocusLock,
   (const OMX_STRING) "OMX_IndexConfigImageFocusLock"},
  {OMX_IndexConfigCommonFocusRange,
   (const OMX_STRING) "OMX_IndexConfigCommonFocusRange"},
  {OMX_IndexConfigImageFlashStatus,
   (const OMX_STRING) "OMX_IndexConfigImageFlashStatus"},
  {OMX_IndexConfigCommonExtCaptureMode,
   (const OMX_STRING) "OMX_IndexConfigCommonExtCaptureMode"},
  {OMX_IndexConfigCommonNDFilterControl,
   (const OMX_STRING) "OMX_IndexConfigCommonNDFilterControl"},
  {OMX_IndexConfigCommonAFAssistantLight,
   (const OMX_STRING) "OMX_IndexConfigCommonAFAssistantLight"},
  {OMX_IndexConfigCommonFocusRegionStatus,
   (const OMX_STRING) "OMX_IndexConfigCommonFocusRegionStatus"},
  {OMX_IndexConfigCommonFocusRegionControl,
   (const OMX_STRING) "OMX_IndexConfigCommonFocusRegionControl"},
  {OMX_IndexParamInterlaceFormat,
   (const OMX_STRING) "OMX_IndexParamInterlaceFormat"},
  {OMX_IndexConfigDeInterlace, (const OMX_STRING) "OMX_IndexConfigDeInterlace"},
  {OMX_IndexConfigStreamInterlaceFormats,
   (const OMX_STRING) "OMX_IndexConfigStreamInterlaceFormats"},

  /* Reserved Configuration range */
  {OMX_IndexOtherStartUnused, (const OMX_STRING) "OMX_IndexOtherStartUnused"},
  {OMX_IndexParamOtherPortFormat,
   (const OMX_STRING) "OMX_IndexParamOtherPortFormat"},
  {OMX_IndexConfigOtherPower, (const OMX_STRING) "OMX_IndexConfigOtherPower"},
  {OMX_IndexConfigOtherStats, (const OMX_STRING) "OMX_IndexConfigOtherStats"},

  /* Reserved Time range */
  {OMX_IndexTimeStartUnused, (const OMX_STRING) "OMX_IndexTimeStartUnused"},
  {OMX_IndexConfigTimeScale, (const OMX_STRING) "OMX_IndexConfigTimeScale"},
  {OMX_IndexConfigTimeClockState,
   (const OMX_STRING) "OMX_IndexConfigTimeClockState"},
  {OMX_IndexReserved_0x90000003,
   (const OMX_STRING) "OMX_IndexReserved_0x90000003"},
  {OMX_IndexConfigTimeCurrentMediaTime,
   (const OMX_STRING) "OMX_IndexConfigTimeCurrentMediaTime"},
  {OMX_IndexConfigTimeCurrentWallTime,
   (const OMX_STRING) "OMX_IndexConfigTimeCurrentWallTime"},
  {OMX_IndexReserved_0x09000006,
   (const OMX_STRING) "OMX_IndexReserved_0x09000006"},
  {OMX_IndexReserved_0x09000007,
   (const OMX_STRING) "OMX_IndexReserved_0x09000007"},
  {OMX_IndexConfigTimeMediaTimeRequest,
   (const OMX_STRING) "OMX_IndexConfigTimeMediaTimeRequest"},
  {OMX_IndexConfigTimeClientStartTime,
   (const OMX_STRING) "OMX_IndexConfigTimeClientStartTime"},
  {OMX_IndexConfigTimePosition,
   (const OMX_STRING) "OMX_IndexConfigTimePosition"},
  {OMX_IndexConfigTimeSeekMode,
   (const OMX_STRING) "OMX_IndexConfigTimeSeekMode"},
  {OMX_IndexConfigTimeCurrentReference,
   (const OMX_STRING) "OMX_IndexConfigTimeCurrentReference"},
  {OMX_IndexConfigTimeActiveRefClockUpdate,
   (const OMX_STRING) "OMX_IndexConfigTimeActiveRefClockUpdate"},
  {OMX_IndexConfigTimeRenderingDelay,
   (const OMX_STRING) "OMX_IndexConfigTimeRenderingDelay"},
  {OMX_IndexConfigTimeUpdate, (const OMX_STRING) "OMX_IndexConfigTimeUpdate"},

  /* Common or Domain Independent Time range */
  {OMX_IndexCommonIndependentStartUnused,
   (const OMX_STRING) "OMX_IndexCommonIndependentStartUnused"},
  {OMX_IndexConfigCommitMode, (const OMX_STRING) "OMX_IndexConfigCommitMode"},
  {OMX_IndexConfigCommit, (const OMX_STRING) "OMX_IndexConfigCommit"},
  {OMX_IndexConfigCallbackRequest,
   (const OMX_STRING) "OMX_IndexConfigCallbackRequest"},
  {OMX_IndexParamMediaContainer,
   (const OMX_STRING) "OMX_IndexParamMediaContainer"},
  {OMX_IndexParamReadOnlyBuffers,
   (const OMX_STRING) "OMX_IndexParamReadOnlyBuffers"},
  {OMX_IndexVendorStartUnused, (const OMX_STRING) "OMX_IndexVendorStartUnused"},
  {OMX_TizoniaIndexParamBufferPreAnnouncementsMode,
   (const OMX_STRING) "OMX_TizoniaIndexParamBufferPreAnnouncementsMode"},
  {OMX_TizoniaIndexParamHttpServer,
   (const OMX_STRING) "OMX_TizoniaIndexParamHttpServer"},
  {OMX_TizoniaIndexParamIcecastMountpoint,
   (const OMX_STRING) "OMX_TizoniaIndexParamIcecastMountpoint"},
  {OMX_TizoniaIndexConfigIcecastMetadata,
   (const OMX_STRING) "OMX_TizoniaIndexConfigIcecastMetadata"},
  {OMX_TizoniaIndexParamAudioOpus,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioOpus"},
  {OMX_TizoniaIndexParamAudioFlac,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioFlac"},
  {OMX_TizoniaIndexParamAudioMp2,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioMp2"},
  {OMX_TizoniaIndexParamAudioSpotifySession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioSpotifySession"},
  {OMX_TizoniaIndexParamAudioSpotifyPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioSpotifyPlaylist"},
  {OMX_TizoniaIndexParamAudioGmusicSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioGmusicSession"},
  {OMX_TizoniaIndexParamAudioGmusicPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioGmusicPlaylist"},
  {OMX_TizoniaIndexConfigPlaylistSkip,
   (const OMX_STRING) "OMX_TizoniaIndexConfigPlaylistSkip"},
  {OMX_TizoniaIndexParamAudioSoundCloudSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioSoundCloudSession"},
  {OMX_TizoniaIndexParamAudioSoundCloudPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioSoundCloudPlaylist"},
  {OMX_TizoniaIndexParamAudioTuneinSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioTuneinSession"},
  {OMX_TizoniaIndexParamAudioTuneinPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioTuneinPlaylist"},
  {OMX_TizoniaIndexParamAudioYoutubeSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioYoutubeSession"},
  {OMX_TizoniaIndexParamAudioYoutubePlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioYoutubePlaylist"},
  {OMX_TizoniaIndexParamAudioDeezerSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioDeezerSession"},
  {OMX_TizoniaIndexParamAudioDeezerPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioDeezerPlaylist"},
  {OMX_TizoniaIndexParamChromecastSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamChromecastSession"},
  {OMX_TizoniaIndexParamAudioPlexSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioPlexSession"},
  {OMX_TizoniaIndexParamAudioPlexPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioPlexPlaylist"},
  {OMX_TizoniaIndexParamAudioIheartSession,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioIheartSession"},
  {OMX_TizoniaIndexParamAudioIheartPlaylist,
   (const OMX_STRING) "OMX_TizoniaIndexParamAudioIheartPlaylist"},
  {OMX_TizoniaIndexConfigPlaylistPosition,
   (const OMX_STRING) "OMX_TizoniaIndexConfigPlaylistPosition"},
  {OMX_IndexKhronosExtensions, (const OMX_STRING) "OMX_IndexKhronosExtensions"},
  {OMX_IndexVendorStartUnused, (const OMX_STRING) "OMX_IndexVendorStartUnused"},
  {OMX_IndexMax, (const OMX_STRING) "OMX_IndexMax"}};

typedef struct tiz_audio_coding_str tiz_audio_coding_str_t;
struct tiz_audio_coding_str
{
  OMX_AUDIO_CODINGTYPE cod;
  const OMX_STRING str;
};

static const tiz_audio_coding_str_t tiz_audio_coding_to_str_tbl[] = {
  {OMX_AUDIO_CodingUnused, (const OMX_STRING) "OMX_AUDIO_CodingUnused"},
  {OMX_AUDIO_CodingAutoDetect, (const OMX_STRING) "OMX_AUDIO_CodingAutoDetect"},
  {OMX_AUDIO_CodingPCM, (const OMX_STRING) "OMX_AUDIO_CodingPCM"},
  {OMX_AUDIO_CodingADPCM, (const OMX_STRING) "OMX_AUDIO_CodingADPCM"},
  {OMX_AUDIO_CodingAMR, (const OMX_STRING) "OMX_AUDIO_CodingAMR"},
  {OMX_AUDIO_CodingGSMFR, (const OMX_STRING) "OMX_AUDIO_CodingGSMFR"},
  {OMX_AUDIO_CodingGSMEFR, (const OMX_STRING) "OMX_AUDIO_CodingGSMEFR"},
  {OMX_AUDIO_CodingGSMHR, (const OMX_STRING) "OMX_AUDIO_CodingGSMHR"},
  {OMX_AUDIO_CodingPDCFR, (const OMX_STRING) "OMX_AUDIO_CodingPDCFR"},
  {OMX_AUDIO_CodingPDCEFR, (const OMX_STRING) "OMX_AUDIO_CodingPDCEFR"},
  {OMX_AUDIO_CodingPDCHR, (const OMX_STRING) "OMX_AUDIO_CodingPDCHR"},
  {OMX_AUDIO_CodingTDMAFR, (const OMX_STRING) "OMX_AUDIO_CodingTDMAFR"},
  {OMX_AUDIO_CodingTDMAEFR, (const OMX_STRING) "OMX_AUDIO_CodingTDMAEFR"},
  {OMX_AUDIO_CodingQCELP8, (const OMX_STRING) "OMX_AUDIO_CodingQCELP8"},
  {OMX_AUDIO_CodingQCELP13, (const OMX_STRING) "OMX_AUDIO_CodingQCELP13"},
  {OMX_AUDIO_CodingEVRC, (const OMX_STRING) "OMX_AUDIO_CodingEVRC"},
  {OMX_AUDIO_CodingSMV, (const OMX_STRING) "OMX_AUDIO_CodingSMV"},
  {OMX_AUDIO_CodingG711, (const OMX_STRING) "OMX_AUDIO_CodingG711"},
  {OMX_AUDIO_CodingG723, (const OMX_STRING) "OMX_AUDIO_CodingG723"},
  {OMX_AUDIO_CodingG726, (const OMX_STRING) "OMX_AUDIO_CodingG726"},
  {OMX_AUDIO_CodingG729, (const OMX_STRING) "OMX_AUDIO_CodingG729"},
  {OMX_AUDIO_CodingAAC, (const OMX_STRING) "OMX_AUDIO_CodingAAC"},
  {OMX_AUDIO_CodingMP3, (const OMX_STRING) "OMX_AUDIO_CodingMP3"},
  {OMX_AUDIO_CodingSBC, (const OMX_STRING) "OMX_AUDIO_CodingSBC"},
  {OMX_AUDIO_CodingVORBIS, (const OMX_STRING) "OMX_AUDIO_CodingVORBIS"},
  {OMX_AUDIO_CodingWMA, (const OMX_STRING) "OMX_AUDIO_CodingWMA"},
  {OMX_AUDIO_CodingRA, (const OMX_STRING) "OMX_AUDIO_CodingRA"},
  {OMX_AUDIO_CodingMIDI, (const OMX_STRING) "OMX_AUDIO_CodingMIDI"},
  {OMX_AUDIO_CodingKhronosExtensions,
   (const OMX_STRING) "OMX_AUDIO_CodingKhronosExtensions"},
  {OMX_AUDIO_CodingVendorStartUnused,
   (const OMX_STRING) "OMX_AUDIO_CodingVendorStartUnused"},
  {OMX_AUDIO_CodingOPUS, (const OMX_STRING) "OMX_AUDIO_CodingOPUS"},
  {OMX_AUDIO_CodingFLAC, (const OMX_STRING) "OMX_AUDIO_CodingFLAC"},
  {OMX_AUDIO_CodingSPEEX, (const OMX_STRING) "OMX_AUDIO_CodingSPEEX"},
  {OMX_AUDIO_CodingOGA, (const OMX_STRING) "OMX_AUDIO_CodingOGA"},
  {OMX_AUDIO_CodingMP2, (const OMX_STRING) "OMX_AUDIO_CodingMP2"},
  {OMX_AUDIO_CodingMP4, (const OMX_STRING) "OMX_AUDIO_CodingMP4"},
  {OMX_AUDIO_CodingWEBM, (const OMX_STRING) "OMX_AUDIO_CodingWEBM"},
  {OMX_AUDIO_CodingMax, (const OMX_STRING) "OMX_AUDIO_CodingMax"}};

/*@observer@*/ OMX_STRING
tiz_cmd_to_str (OMX_COMMANDTYPE a_cmd)
{
  const size_t count = sizeof (tiz_cmd_to_str_tbl) / sizeof (tiz_cmd_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_cmd_to_str_tbl[i].cmd == a_cmd)
        {
          return tiz_cmd_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL command";
}

/*@observer@*/ OMX_STRING
tiz_state_to_str (OMX_STATETYPE a_id)
{
  const size_t count = sizeof (tiz_state_to_str_tbl) / sizeof (tiz_state_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_state_to_str_tbl[i].state == a_id)
        {
          return tiz_state_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL state";
}

/*@observer@*/ OMX_STRING
tiz_evt_to_str (OMX_EVENTTYPE a_evt)
{
  const size_t count = sizeof (tiz_evt_to_str_tbl) / sizeof (tiz_evt_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_evt_to_str_tbl[i].evt == a_evt)
        {
          return tiz_evt_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL event";
}

/*@observer@*/ OMX_STRING
tiz_err_to_str (OMX_ERRORTYPE a_err)
{
  const size_t count = sizeof (tiz_err_to_str_tbl) / sizeof (tiz_err_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_err_to_str_tbl[i].err == a_err)
        {
          return tiz_err_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL error";
}

/*@observer@*/ OMX_STRING
tiz_dir_to_str (OMX_DIRTYPE a_dir)
{
  const size_t count = sizeof (tiz_dir_to_str_tbl) / sizeof (tiz_dir_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_dir_to_str_tbl[i].dir == a_dir)
        {
          return tiz_dir_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL port direction";
}

/*@observer@*/ OMX_STRING
tiz_domain_to_str (OMX_PORTDOMAINTYPE a_domain)
{
  const size_t count = sizeof (tiz_domain_to_str_tbl) / sizeof (tiz_domain_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_domain_to_str_tbl[i].domain == a_domain)
        {
          return tiz_domain_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL port domain";
}

/*@observer@*/ OMX_STRING
tiz_idx_to_str (OMX_INDEXTYPE a_idx)
{
  const size_t count = sizeof (tiz_idx_to_str_tbl) / sizeof (tiz_idx_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_idx_to_str_tbl[i].idx == a_idx)
        {
          return tiz_idx_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL index";
}

/*@observer@*/ OMX_STRING
tiz_audio_coding_to_str (OMX_AUDIO_CODINGTYPE a_cod)
{
  const size_t count
    = sizeof (tiz_audio_coding_to_str_tbl) / sizeof (tiz_audio_coding_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tiz_audio_coding_to_str_tbl[i].cod == a_cod)
        {
          return tiz_audio_coding_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown OpenMAX IL Audio Coding Type";
}

void
tiz_util_reset_eos_flag (OMX_BUFFERHEADERTYPE * p_hdr)
{
  assert (p_hdr);
  p_hdr->nFlags &= ~(1 << OMX_BUFFERFLAG_EOS);
}

void
tiz_util_set_eos_flag (OMX_BUFFERHEADERTYPE * p_hdr)
{
  assert (p_hdr);
  p_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
}
