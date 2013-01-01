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
 *  OMX_Other.h - OpenMax IL version 1.2.0
 *  The structures needed by Other components to exchange
 *  parameters and configuration data with the components.
 */

#ifndef OMX_Other_h
#define OMX_Other_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header must include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully 
 */

#include <OMX_Core.h>

typedef enum OMX_OTHER_FORMATTYPE {
    OMX_OTHER_FormatTime = 0,
    OMX_OTHER_FormatPower,
    OMX_OTHER_FormatStats,
    OMX_OTHER_FormatBinary,
    OMX_OTHER_FormatVendorReserved = 1000, /**< Starting value for vendor specific 
                                                formats */

    OMX_OTHER_FormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_OTHER_FormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_OTHER_FormatMax = 0x7FFFFFFF
} OMX_OTHER_FORMATTYPE;

typedef enum OMX_TIME_SEEKMODETYPE {
    OMX_TIME_SeekModeFast = 0,
    OMX_TIME_SeekModeAccurate,
    OMX_TIME_SeekModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_TIME_SeekModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_TIME_SeekModeMax = 0x7FFFFFFF
} OMX_TIME_SEEKMODETYPE;

typedef struct OMX_TIME_CONFIG_SEEKMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIME_SEEKMODETYPE eType;
} OMX_TIME_CONFIG_SEEKMODETYPE;

typedef struct OMX_TIME_CONFIG_TIMESTAMPTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TICKS nTimestamp;
} OMX_TIME_CONFIG_TIMESTAMPTYPE;  

typedef enum OMX_TIME_UPDATETYPE {
      OMX_TIME_UpdateRequestFulfillment,
      OMX_TIME_UpdateScaleChanged,
      OMX_TIME_UpdateClockStateChanged,
      OMX_TIME_UpdateKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
      OMX_TIME_UpdateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
      OMX_TIME_UpdateMax = 0x7FFFFFFF
} OMX_TIME_UPDATETYPE;

typedef enum OMX_TIME_REFCLOCKTYPE {
      OMX_TIME_RefClockNone,
      OMX_TIME_RefClockAudio,
      OMX_TIME_RefClockVideo,
      OMX_TIME_RefClockKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
      OMX_TIME_RefClockVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
      OMX_TIME_RefClockMax = 0x7FFFFFFF
} OMX_TIME_REFCLOCKTYPE;

typedef enum OMX_TIME_CLOCKSTATE {
      OMX_TIME_ClockStateRunning,
      OMX_TIME_ClockStateWaitingForStartTime,
      OMX_TIME_ClockStateStopped,
      OMX_TIME_ClockStateKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
      OMX_TIME_ClockStateVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
      OMX_TIME_ClockStateMax = 0x7FFFFFFF
} OMX_TIME_CLOCKSTATE;

typedef struct OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_PTR pClientPrivate;
    OMX_TICKS nMediaTimestamp;
    OMX_TICKS nOffset;
} OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE;

typedef struct OMX_TIME_MEDIATIMETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nClientPrivate;
    OMX_TIME_UPDATETYPE eUpdateType;
    OMX_TICKS nMediaTimestamp;
    OMX_TICKS nOffset;
    OMX_TICKS nWallTimeAtMediaTime;
    OMX_S32 xScale;
    OMX_TIME_CLOCKSTATE eState;
} OMX_TIME_MEDIATIMETYPE;  

typedef struct OMX_TIME_CONFIG_SCALETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_S32 xScale;
} OMX_TIME_CONFIG_SCALETYPE;
 
#define OMX_CLOCKPORT0 0x00000001
#define OMX_CLOCKPORT1 0x00000002
#define OMX_CLOCKPORT2 0x00000004
#define OMX_CLOCKPORT3 0x00000008
#define OMX_CLOCKPORT4 0x00000010
#define OMX_CLOCKPORT5 0x00000020
#define OMX_CLOCKPORT6 0x00000040
#define OMX_CLOCKPORT7 0x00000080

typedef struct OMX_TIME_CONFIG_CLOCKSTATETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_TIME_CLOCKSTATE eState;
    OMX_TICKS nStartTime;
    OMX_TICKS nOffset;
    OMX_U32 nWaitMask;
} OMX_TIME_CONFIG_CLOCKSTATETYPE;

typedef struct OMX_OTHER_CONFIG_POWERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bEnablePM;
} OMX_OTHER_CONFIG_POWERTYPE;

typedef struct OMX_OTHER_CONFIG_STATSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    /* what goes here */
} OMX_OTHER_CONFIG_STATSTYPE;

typedef struct OMX_OTHER_PORTDEFINITIONTYPE {
    OMX_OTHER_FORMATTYPE eFormat;
} OMX_OTHER_PORTDEFINITIONTYPE;

typedef struct OMX_OTHER_PARAM_PORTFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_OTHER_FORMATTYPE eFormat;
} OMX_OTHER_PARAM_PORTFORMATTYPE; 

typedef struct OMX_TIME_CONFIG_ACTIVEREFCLOCKUPDATETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bEnableRefClockUpdates;
    OMX_TICKS nRefTimeUpdateInterval;
} OMX_TIME_CONFIG_ACTIVEREFCLOCKUPDATETYPE;

typedef struct OMX_TIME_CONFIG_RENDERINGDELAYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TICKS nRenderingDelay;
} OMX_TIME_CONFIG_RENDERINGDELAYTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
