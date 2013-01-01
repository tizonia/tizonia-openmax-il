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
 *  OMX_IVCommon.h - OpenMax IL version 1.2.0
 *  The structures needed by Video and Image components to exchange
 *  parameters and configuration data with the components.
 */

#ifndef OMX_IVCommon_h
#define OMX_IVCommon_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Each OMX header must include all required header files to allow the header
 * to compile without errors.  The includes below are required for this header
 * file to compile successfully 
 */

#include <OMX_Core.h>

typedef enum OMX_COLOR_FORMATTYPE {
    OMX_COLOR_FormatUnused,
    OMX_COLOR_FormatMonochrome,
    OMX_COLOR_Format8bitRGB332,
    OMX_COLOR_Format12bitRGB444,
    OMX_COLOR_Format16bitARGB4444,
    OMX_COLOR_Format16bitARGB1555,
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_Format16bitBGR565,
    OMX_COLOR_Format18bitRGB666,
    OMX_COLOR_Format18bitARGB1665,
    OMX_COLOR_Format19bitARGB1666, 
    OMX_COLOR_Format24bitRGB888,
    OMX_COLOR_Format24bitBGR888,
    OMX_COLOR_Format24bitARGB1887,
    OMX_COLOR_Format25bitARGB1888,
    OMX_COLOR_Format32bitBGRA8888,
    OMX_COLOR_Format32bitARGB8888,
    OMX_COLOR_FormatYUV411Planar,
    OMX_COLOR_FormatYUV411PackedPlanar,
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatYUV420PackedPlanar,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV422Planar,
    OMX_COLOR_FormatYUV422PackedPlanar,
    OMX_COLOR_FormatYUV422SemiPlanar,
    OMX_COLOR_FormatYCbYCr,
    OMX_COLOR_FormatYCrYCb,
    OMX_COLOR_FormatCbYCrY,
    OMX_COLOR_FormatCrYCbY,
    OMX_COLOR_FormatYUV444Interleaved,
    OMX_COLOR_FormatRawBayer8bit,
    OMX_COLOR_FormatRawBayer10bit,
    OMX_COLOR_FormatRawBayer8bitcompressed,
    OMX_COLOR_FormatL2, 
    OMX_COLOR_FormatL4, 
    OMX_COLOR_FormatL8, 
    OMX_COLOR_FormatL16, 
    OMX_COLOR_FormatL24, 
    OMX_COLOR_FormatL32,
    OMX_COLOR_FormatYUV420PackedSemiPlanar,
    OMX_COLOR_FormatYUV422PackedSemiPlanar,
    OMX_COLOR_Format18BitBGR666,
    OMX_COLOR_Format24BitARGB6666,
    OMX_COLOR_Format24BitABGR6666,
    OMX_COLOR_Format32bitABGR8888,
    OMX_COLOR_FormatYVU420Planar,
    OMX_COLOR_FormatYVU420PackedPlanar,
    OMX_COLOR_FormatYVU420SemiPlanar,
    OMX_COLOR_FormatYVU420PackedSemiPlanar,
    OMX_COLOR_FormatYVU422Planar,
    OMX_COLOR_FormatYVU422PackedPlanar,
    OMX_COLOR_FormatYVU422SemiPlanar,
    OMX_COLOR_FormatYVU422PackedSemiPlanar,
    OMX_COLOR_Format8bitBGR233,
    OMX_COLOR_Format12bitBGR444,
    OMX_COLOR_Format16bitBGRA4444,
    OMX_COLOR_Format16bitBGRA5551,
    OMX_COLOR_Format18bitBGRA5661,
    OMX_COLOR_Format19bitBGRA6661,
    OMX_COLOR_Format24bitBGRA7881,
    OMX_COLOR_Format25bitBGRA8881,
    OMX_COLOR_Format24BitBGRA6666,
    OMX_COLOR_Format24BitRGBA6666,
    OMX_COLOR_FormatKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_COLOR_FormatVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_COLOR_FormatMax = 0x7FFFFFFF
} OMX_COLOR_FORMATTYPE;

typedef struct OMX_CONFIG_COLORCONVERSIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xColorMatrix[3][3];
    OMX_S32 xColorOffset[4];
}OMX_CONFIG_COLORCONVERSIONTYPE;

typedef struct OMX_CONFIG_SCALEFACTORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 xWidth;
    OMX_S32 xHeight;
}OMX_CONFIG_SCALEFACTORTYPE;

typedef enum OMX_IMAGEFILTERTYPE {
    OMX_ImageFilterNone,
    OMX_ImageFilterNoise,
    OMX_ImageFilterEmboss,
    OMX_ImageFilterNegative,
    OMX_ImageFilterSketch,
    OMX_ImageFilterOilPaint,
    OMX_ImageFilterHatch,
    OMX_ImageFilterGpen,
    OMX_ImageFilterAntialias, 
    OMX_ImageFilterDeRing,       
    OMX_ImageFilterSolarize,
    OMX_ImageFilterPastel,
    OMX_ImageFilterMosaic,
    OMX_ImageFilterPosterize,
    OMX_ImageFilterWhiteboard,
    OMX_ImageFilterBlackboard,
    OMX_ImageFilterSepia,
    OMX_ImageFilterGrayscale,
    OMX_ImageFilterNatural,
    OMX_ImageFilterVivid,
    OMX_ImageFilterWaterColor,
    OMX_ImageFilterFilm,
    OMX_ImageFilterKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_ImageFilterVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_ImageFilterMax = 0x7FFFFFFF
} OMX_IMAGEFILTERTYPE;

typedef struct OMX_CONFIG_IMAGEFILTERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGEFILTERTYPE eImageFilter;
} OMX_CONFIG_IMAGEFILTERTYPE;

typedef struct OMX_CONFIG_COLORENHANCEMENTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;
    OMX_BOOL bColorEnhancement;
    OMX_U8 nCustomizedU;
    OMX_U8 nCustomizedV;
} OMX_CONFIG_COLORENHANCEMENTTYPE;

typedef struct OMX_CONFIG_COLORKEYTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nARGBColor;
    OMX_U32 nARGBMask;
} OMX_CONFIG_COLORKEYTYPE;

typedef enum OMX_COLORBLENDTYPE {
    OMX_ColorBlendNone,
    OMX_ColorBlendAlphaConstant,
    OMX_ColorBlendAlphaPerPixel,
    OMX_ColorBlendAlternate,
    OMX_ColorBlendAnd,
    OMX_ColorBlendOr,
    OMX_ColorBlendInvert,
    OMX_ColorBlendKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_ColorBlendVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_ColorBlendMax = 0x7FFFFFFF
} OMX_COLORBLENDTYPE;

typedef struct OMX_CONFIG_COLORBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nRGBAlphaConstant;
    OMX_COLORBLENDTYPE  eColorBlend;
} OMX_CONFIG_COLORBLENDTYPE;

typedef struct OMX_FRAMESIZETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_FRAMESIZETYPE;

typedef struct OMX_CONFIG_ROTATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nRotation; 
} OMX_CONFIG_ROTATIONTYPE;

typedef enum OMX_MIRRORTYPE {
    OMX_MirrorNone = 0,
    OMX_MirrorVertical,
    OMX_MirrorHorizontal,
    OMX_MirrorBoth, 
    OMX_MirrorKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_MirrorVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_MirrorMax = 0x7FFFFFFF   
} OMX_MIRRORTYPE;

typedef struct OMX_CONFIG_MIRRORTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;
    OMX_MIRRORTYPE  eMirror;
} OMX_CONFIG_MIRRORTYPE;

typedef struct OMX_CONFIG_POINTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nX;
    OMX_S32 nY;
} OMX_CONFIG_POINTTYPE;

typedef struct OMX_CONFIG_RECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;  
    OMX_U32 nPortIndex; 
    OMX_S32 nLeft; 
    OMX_S32 nTop;
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} OMX_CONFIG_RECTTYPE;

typedef struct OMX_PARAM_DEBLOCKINGTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDeblocking;
} OMX_PARAM_DEBLOCKINGTYPE;

typedef struct OMX_CONFIG_FRAMESTABTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStab;
} OMX_CONFIG_FRAMESTABTYPE;

typedef enum OMX_WHITEBALCONTROLTYPE {
    OMX_WhiteBalControlOff = 0,
    OMX_WhiteBalControlAuto,
    OMX_WhiteBalControlSunLight,
    OMX_WhiteBalControlCloudy,
    OMX_WhiteBalControlShade,
    OMX_WhiteBalControlTungsten,
    OMX_WhiteBalControlFluorescent,
    OMX_WhiteBalControlIncandescent,
    OMX_WhiteBalControlFlash,
    OMX_WhiteBalControlHorizon,
    OMX_WhiteBalControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_WhiteBalControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_WhiteBalControlMax = 0x7FFFFFFF
} OMX_WHITEBALCONTROLTYPE;

typedef struct OMX_CONFIG_WHITEBALCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_WHITEBALCONTROLTYPE eWhiteBalControl;
} OMX_CONFIG_WHITEBALCONTROLTYPE;

typedef enum OMX_EXPOSURECONTROLTYPE {
    OMX_ExposureControlOff = 0,
    OMX_ExposureControlAuto,
    OMX_ExposureControlNight,
    OMX_ExposureControlBackLight,
    OMX_ExposureControlSpotLight,
    OMX_ExposureControlSports,
    OMX_ExposureControlSnow,
    OMX_ExposureControlBeach,
    OMX_ExposureControlLargeAperture,
    OMX_ExposureControlSmallApperture,
    OMX_ExposureControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_ExposureControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_ExposureControlMax = 0x7FFFFFFF
} OMX_EXPOSURECONTROLTYPE;

typedef struct OMX_CONFIG_EXPOSURECONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_EXPOSURECONTROLTYPE eExposureControl;
} OMX_CONFIG_EXPOSURECONTROLTYPE;

typedef struct OMX_PARAM_SENSORMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameRate;
    OMX_BOOL bOneShot;
    OMX_FRAMESIZETYPE sFrameSize;
} OMX_PARAM_SENSORMODETYPE;

typedef struct OMX_CONFIG_CONTRASTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nContrast;
} OMX_CONFIG_CONTRASTTYPE;

typedef struct OMX_CONFIG_BRIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBrightness;
} OMX_CONFIG_BRIGHTNESSTYPE;

typedef struct OMX_CONFIG_BACKLIGHTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBacklight;
    OMX_U32 nTimeout;
} OMX_CONFIG_BACKLIGHTTYPE;

typedef struct OMX_CONFIG_GAMMATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nGamma;
} OMX_CONFIG_GAMMATYPE;

typedef struct OMX_CONFIG_SATURATIONTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nSaturation;
} OMX_CONFIG_SATURATIONTYPE;

typedef struct OMX_CONFIG_LIGHTNESSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nLightness;
} OMX_CONFIG_LIGHTNESSTYPE;

typedef struct OMX_CONFIG_PLANEBLENDTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nDepth;
    OMX_U32 nAlpha;
} OMX_CONFIG_PLANEBLENDTYPE;

typedef struct OMX_PARAM_INTERLEAVETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_U32 nInterleavePortIndex;
} OMX_PARAM_INTERLEAVETYPE;

typedef enum OMX_TRANSITIONEFFECTTYPE {
    OMX_EffectNone,
    OMX_EffectFadeFromBlack,
    OMX_EffectFadeToBlack,
    OMX_EffectUnspecifiedThroughConstantColor,
    OMX_EffectDissolve,
    OMX_EffectWipe,
    OMX_EffectUnspecifiedMixOfTwoScenes,
    OMX_EffectKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_EffectVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_EffectMax = 0x7FFFFFFF
} OMX_TRANSITIONEFFECTTYPE;

typedef struct OMX_CONFIG_TRANSITIONEFFECTTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_TRANSITIONEFFECTTYPE eEffect;
} OMX_CONFIG_TRANSITIONEFFECTTYPE;

typedef enum OMX_DATAUNITTYPE {
    OMX_DataUnitCodedPicture,
    OMX_DataUnitVideoSegment,
    OMX_DataUnitSeveralSegments,
    OMX_DataUnitArbitraryStreamSection,
    OMX_DataUnitKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_DataUnitVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_DataUnitMax = 0x7FFFFFFF
} OMX_DATAUNITTYPE;

typedef enum OMX_DATAUNITENCAPSULATIONTYPE {
    OMX_DataEncapsulationElementaryStream,
    OMX_DataEncapsulationGenericPayload,
    OMX_DataEncapsulationRtpPayload,
    OMX_DataEncapsulationKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_DataEncapsulationVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_DataEncapsulationMax = 0x7FFFFFFF
} OMX_DATAUNITENCAPSULATIONTYPE;

typedef struct OMX_PARAM_DATAUNITTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_DATAUNITTYPE eUnitType;
    OMX_DATAUNITENCAPSULATIONTYPE eEncapsulationType;
} OMX_PARAM_DATAUNITTYPE;

typedef enum OMX_DITHERTYPE {
    OMX_DitherNone,
    OMX_DitherOrdered,
    OMX_DitherErrorDiffusion,
    OMX_DitherOther,
    OMX_DitherKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_DitherVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_DitherMax = 0x7FFFFFFF
} OMX_DITHERTYPE;

typedef struct OMX_CONFIG_DITHERTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_DITHERTYPE eDither;
} OMX_CONFIG_DITHERTYPE;

typedef struct OMX_CONFIG_CAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bContinuous;
    OMX_BOOL bFrameLimited;
    OMX_U32 nFrameLimit;
} OMX_CONFIG_CAPTUREMODETYPE;

typedef enum OMX_METERINGTYPE {
    OMX_MeteringModeAverage,
    OMX_MeteringModeSpot,
    OMX_MeteringModeMatrix,
    OMX_MeteringKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_MeteringVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_EVModeMax = 0x7fffffff
} OMX_METERINGTYPE;
 
typedef struct OMX_CONFIG_EXPOSUREVALUETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_METERINGTYPE eMetering;
    OMX_S32 xEVCompensation;
    OMX_U32 nApertureFNumber;
    OMX_BOOL bAutoAperture;
    OMX_U32 nShutterSpeedMsec;
    OMX_BOOL bAutoShutterSpeed;
    OMX_U32 nSensitivity;
    OMX_BOOL bAutoSensitivity;
} OMX_CONFIG_EXPOSUREVALUETYPE;

typedef enum OMX_FOCUSSTATUSTYPE {
    OMX_FocusStatusOff = 0,
    OMX_FocusStatusRequest,
    OMX_FocusStatusReached,
    OMX_FocusStatusUnableToReach,
    OMX_FocusStatusLost,
    OMX_FocusStatusKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FocusStatusVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FocusStatusMax = 0x7FFFFFFF
} OMX_FOCUSSTATUSTYPE;

typedef struct OMX_SHARPNESSTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nSharpness;
} OMX_SHARPNESSTYPE;

typedef struct OMX_CONFIG_ZOOMFACTORTYPE { 
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex; 
    OMX_BU32 xZoomFactor; 
}OMX_CONFIG_ZOOMFACTORTYPE;

typedef enum OMX_IMAGE_LOCKTYPE {
    OMX_IMAGE_LockOff = 0, 
    OMX_IMAGE_LockImmediate,
    OMX_IMAGE_LockAtCapture,
    OMX_IMAGE_LockKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_LockVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_LockMax = 0x7FFFFFFF
} OMX_IMAGE_LOCKTYPE;

typedef struct OMX_IMAGE_CONFIG_LOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_LOCKTYPE eImageLock;
} OMX_IMAGE_CONFIG_LOCKTYPE;

typedef enum OMX_FOCUSRANGETYPE {
    OMX_FocusRangeAuto = 0, 
    OMX_FocusRangeHyperfocal,
    OMX_FocusRangeNormal,
    OMX_FocusRangeSuperMacro,
    OMX_FocusRangeMacro,
    OMX_FocusRangeInfinity,
    OMX_FocusRangeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FocusRangeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FocusRangeMax = 0x7FFFFFFF
} OMX_FOCUSRANGETYPE;

typedef struct OMX_CONFIG_FOCUSRANGETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FOCUSRANGETYPE eFocusRange;
} OMX_CONFIG_FOCUSRANGETYPE;

typedef enum OMX_IMAGE_FLASHSTATUSTYPE
{
    OMX_IMAGE_FlashUnknown 	= 0,
    OMX_IMAGE_FlashOff,
    OMX_IMAGE_FlashCharging,
    OMX_IMAGE_FlashReady,
    OMX_IMAGE_FlashNotAvailable,
    OMX_IMAGE_FlashInsufficientCharge,
    OMX_IMAGE_FlashKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_FlashVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_FlashMax = 0x7FFFFFFF
} OMX_IMAGE_FLASHSTATUSTYPE;
              
typedef struct OMX_IMAGE_CONFIG_FLASHSTATUSTYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_IMAGE_FLASHSTATUSTYPE eFlashStatus;
} OMX_IMAGE_CONFIG_FLASHSTATUSTYPE;

typedef struct OMX_CONFIG_EXTCAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameBefore;
    OMX_BOOL bPrepareCapture;
} OMX_CONFIG_EXTCAPTUREMODETYPE;

typedef enum OMX_NDFILTERCONTROLTYPE{
    OMX_NDFilterOff,
    OMX_NDFilterOn,
    OMX_NDFilterAuto,
    OMX_NDFilterKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_NDFilterVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_NDFilterMax = 0x7FFFFFFF
} OMX_NDFILTERCONTROLTYPE;

typedef struct OMX_CONFIG_NDFILTERCONTROLTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_NDFILTERCONTROLTYPE eNDFilterControl;
} OMX_CONFIG_NDFILTERCONTROLTYPE;

typedef enum OMX_AFASSISTANTLIGHTTYPE{
    OMX_AFAssistantLightOff,
    OMX_AFAssistantLightOn,
    OMX_AFAssistantLightAuto,
    OMX_AFAssistantLightKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_AFAssistantLightVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_AFAssistantLightMax = 0x7FFFFFFF
} OMX_AFASSISTANTLIGHTTYPE;

typedef struct OMX_CONFIG_AFASSISTANTLIGHTTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_AFASSISTANTLIGHTTYPE eAFAssistantLight;
} OMX_CONFIG_AFASSISTANTLIGHTTYPE;

#define OMX_InterlaceFrameProgressive                   0x00000001
#define OMX_InterlaceInterleaveFrameTopFieldFirst       0x00000002
#define OMX_InterlaceInterleaveFrameBottomFieldFirst    0x00000004
#define OMX_InterlaceFrameTopFieldFirst                 0x00000008
#define OMX_InterlaceFrameBottomFieldFirst              0x00000010
#define OMX_InterlaceInterleaveFieldTop                 0x00000020
#define OMX_InterlaceInterleaveFieldBottom              0x00000040

typedef struct OMX_INTERLACEFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex; 
    OMX_U32 nFormat;
    OMX_TICKS nTimeStamp;
} OMX_INTERLACEFORMATTYPE;

typedef struct  OMX_DEINTERLACETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
} OMX_DEINTERLACETYPE;

typedef struct  OMX_STREAMINTERLACEFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bInterlaceFormat;
    OMX_U32 nInterlaceFormats;
} OMX_STREAMINTERLACEFORMAT;

typedef struct OMX_FROITYPE {
    OMX_S32 nRectX;
    OMX_S32 nRectY;
    OMX_S32 nRectWidth;
    OMX_S32 nRectHeight;
    OMX_S32 xFocusDistance;
    OMX_FOCUSSTATUSTYPE eFocusStatus;
} OMX_FROITYPE;

typedef struct OMX_CONFIG_FOCUSREGIONSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bFocused;
    OMX_U32 nMaxFAreas;
    OMX_U32 nFAreas;
    OMX_FROITYPE sFROIs[1];
} OMX_CONFIG_FOCUSREGIONSTATUSTYPE;

typedef enum OMX_FOCUSREGIONCONTROLTYPE {
    OMX_FocusRegionControlAuto = 0,
    OMX_FocusRegionControlManual,
    OMX_FocusRegionControlFacePriority,
    OMX_FocusRegionControlObjectPriority,
    OMX_FocusRegionControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_FocusRegionControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FocusRegionControlMax = 0x7FFFFFFF
} OMX_FOCUSREGIONCONTROLTYPE;

typedef struct OMX_MANUALFOCUSRECTTYPE {
    OMX_S32 nRectX;
    OMX_S32 nRectY;
    OMX_S32 nRectWidth;
    OMX_S32 nRectHeight;
} OMX_MANUALFOCUSRECTTYPE;

typedef struct OMX_CONFIG_FOCUSREGIONCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nFAreas;
    OMX_FOCUSREGIONCONTROLTYPE eFocusRegionsControl;
    OMX_MANUALFOCUSRECTTYPE sManualFRegions[1];
} OMX_CONFIG_FOCUSREGIONCONTROLTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
