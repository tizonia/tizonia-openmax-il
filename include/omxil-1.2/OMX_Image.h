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
 * OMX_Image.h - OpenMax IL version 1.2.0
 * The structures needed by Image components to exchange parameters and 
 * configuration data with the components.
 */

#ifndef OMX_Image_h
#define OMX_Image_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Each OMX header must include all required header files to allow the 
 * header to compile without errors.  The includes below are required  
 * for this header file to compile successfully 
 */

#include <OMX_IVCommon.h>

typedef enum OMX_IMAGE_CODINGTYPE {
    OMX_IMAGE_CodingUnused,
    OMX_IMAGE_CodingAutoDetect,
    OMX_IMAGE_CodingJPEG,
    OMX_IMAGE_CodingJPEG2K,
    OMX_IMAGE_CodingEXIF,
    OMX_IMAGE_CodingTIFF,
    OMX_IMAGE_CodingGIF,
    OMX_IMAGE_CodingPNG,
    OMX_IMAGE_CodingLZW,
    OMX_IMAGE_CodingBMP,
    OMX_IMAGE_CodingWEBP,
    OMX_IMAGE_CodingKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_CodingVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_CodingMax = 0x7FFFFFFF
} OMX_IMAGE_CODINGTYPE;

typedef struct OMX_IMAGE_PORTDEFINITIONTYPE {
    OMX_NATIVE_DEVICETYPE pNativeRender;
    OMX_U32 nFrameWidth; 
    OMX_U32 nFrameHeight;
    OMX_S32 nStride;     
    OMX_U32 nSliceHeight;
    OMX_BOOL bFlagErrorConcealment;
    OMX_IMAGE_CODINGTYPE eCompressionFormat;
    OMX_COLOR_FORMATTYPE eColorFormat;
    OMX_NATIVE_WINDOWTYPE pNativeWindow;
} OMX_IMAGE_PORTDEFINITIONTYPE;

typedef struct OMX_IMAGE_PARAM_PORTFORMATTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_IMAGE_CODINGTYPE eCompressionFormat;
    OMX_COLOR_FORMATTYPE eColorFormat;
} OMX_IMAGE_PARAM_PORTFORMATTYPE;

typedef enum OMX_IMAGE_FLASHCONTROLTYPE {
    OMX_IMAGE_FlashControlOn = 0,
    OMX_IMAGE_FlashControlOff,
    OMX_IMAGE_FlashControlAuto,
    OMX_IMAGE_FlashControlRedEyeReduction,
    OMX_IMAGE_FlashControlFillin,
    OMX_IMAGE_FlashControlTorch,
    OMX_IMAGE_FlashControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_FlashControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_FlashControlMax = 0x7FFFFFFF
} OMX_IMAGE_FLASHCONTROLTYPE;

typedef struct OMX_IMAGE_PARAM_FLASHCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_FLASHCONTROLTYPE eFlashControl;
} OMX_IMAGE_PARAM_FLASHCONTROLTYPE;

typedef enum OMX_IMAGE_FOCUSCONTROLTYPE {
    OMX_IMAGE_FocusControlOn = 0,
    OMX_IMAGE_FocusControlOff,
    OMX_IMAGE_FocusControlAuto,
    OMX_IMAGE_FocusControlAutoLock,
    OMX_IMAGE_FocusControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_FocusControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_FocusControlMax = 0x7FFFFFFF
} OMX_IMAGE_FOCUSCONTROLTYPE;

typedef struct OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_FOCUSCONTROLTYPE eFocusControl;
    OMX_U32 nFocusSteps;
    OMX_U32 nFocusStepIndex;
} OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE;

typedef struct OMX_IMAGE_PARAM_QFACTORTYPE {
    OMX_U32 nSize;            
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;       
    OMX_U32 nQFactor;                                        
} OMX_IMAGE_PARAM_QFACTORTYPE;

typedef enum OMX_IMAGE_QUANTIZATIONTABLETYPE {
    OMX_IMAGE_QuantizationTableLuma = 0,
    OMX_IMAGE_QuantizationTableChroma,
    OMX_IMAGE_QuantizationTableChromaCb,
    OMX_IMAGE_QuantizationTableChromaCr,
    OMX_IMAGE_QuantizationTableKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_QuantizationTableVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_QuantizationTableMax = 0x7FFFFFFF
} OMX_IMAGE_QUANTIZATIONTABLETYPE;

typedef struct OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_QUANTIZATIONTABLETYPE eQuantizationTable;
    OMX_U8 nQuantizationMatrix[64];
} OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE;

typedef enum OMX_IMAGE_HUFFMANTABLETYPE {
    OMX_IMAGE_HuffmanTableAC = 0,
    OMX_IMAGE_HuffmanTableDC,
    OMX_IMAGE_HuffmanTableACLuma,
    OMX_IMAGE_HuffmanTableACChroma,
    OMX_IMAGE_HuffmanTableDCLuma,
    OMX_IMAGE_HuffmanTableDCChroma,
    OMX_IMAGE_HuffmanTableKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_HuffmanTableVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_HuffmanTableMax = 0x7FFFFFFF
} OMX_IMAGE_HUFFMANTABLETYPE;

typedef struct OMX_IMAGE_PARAM_HUFFMANTTABLETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_HUFFMANTABLETYPE eHuffmanTable;
    OMX_U8 nNumberOfHuffmanCodeOfLength[16];
    OMX_U8 nHuffmanTable[256];
}OMX_IMAGE_PARAM_HUFFMANTTABLETYPE;

typedef enum OMX_FLICKERREJECTIONTYPE {
    OMX_FlickerRejectionOff = 0,
    OMX_FlickerRejectionAuto,
    OMX_FlickerRejection50,
    OMX_FlickerRejection60,
    OMX_FlickerRejectionKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FlickerRejectionVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FlickerRejectionMax = 0x7FFFFFFF
}OMX_FLICKERREJECTIONTYPE;

typedef struct OMX_CONFIG_FLICKERREJECTIONTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FLICKERREJECTIONTYPE eFlickerRejection;
} OMX_CONFIG_FLICKERREJECTIONTYPE;

typedef enum OMX_HISTOGRAMTYPE {
    OMX_Histogram_Off = 0,
    OMX_Histogram_RGB,
    OMX_Histogram_Luma,
    OMX_Histogram_Chroma,
    OMX_HistogramKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_HistogramVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_HistogramMax = 0x7FFFFFFF
}OMX_HISTOGRAMTYPE;

typedef struct OMX_IMAGE_HISTOGRAMTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nBins;
    OMX_HISTOGRAMTYPE eHistType;
} OMX_IMAGE_HISTOGRAMTYPE;

typedef struct OMX_IMAGE_HISTOGRAMDATATYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_HISTOGRAMTYPE eHistType;
    OMX_U32 nBins;
    OMX_U8 data[1];
} OMX_IMAGE_HISTOGRAMDATATYPE;

typedef struct OMX_IMAGE_HISTOGRAMINFOTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_HISTOGRAMTYPE eHistType;
    OMX_U32 nBins;
    OMX_U16 nBitsPerBin;
} OMX_IMAGE_HISTOGRAMINFOTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
