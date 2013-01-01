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
 *  OMX_Types.h - OpenMax IL version 1.2.0
 *  The OMX_Types header file contains the primitive type
 *  definitions used by the core, the application and the
 *  component.  This file may need to be modified to be used on
 *  systems that do not have "char" set to 8 bits, "short" set
 *  to 16 bits and "long" set to 32 bits.
 */

#ifndef OMX_Types_h
#define OMX_Types_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** The OMX_API and OMX_APIENTRY are platform specific definitions used
 *  to declare OMX function prototypes.  They are modified to meet the
 *  requirements for a particular platform */
#ifdef __SYMBIAN32__   
#   ifdef __OMX_EXPORTS
#       define OMX_API __declspec(dllexport)
#   else
#       ifdef _WIN32
#           define OMX_API __declspec(dllexport) 
#       else
#           define OMX_API __declspec(dllimport)
#       endif
#   endif
#else
#   ifdef _WIN32
#      ifdef __OMX_EXPORTS
#          define OMX_API __declspec(dllexport)
#      else
#          define OMX_API __declspec(dllimport)
#      endif
#   else
#      ifdef __OMX_EXPORTS
#          define OMX_API
#      else
#          define OMX_API extern
#      endif
#   endif
#endif

#ifndef OMX_APIENTRY
#define OMX_APIENTRY 
#endif 

/** OMX_IN is used to identify inputs to an OMX function.  This designation 
    will also be used in the case of a pointer that points to a parameter 
    that is used as an output. */
#ifndef OMX_IN
#define OMX_IN
#endif

/** OMX_OUT is used to identify outputs from an OMX function.  This 
    designation will also be used in the case of a pointer that points 
    to a parameter that is used as an input. */
#ifndef OMX_OUT
#define OMX_OUT
#endif


/** OMX_INOUT is used to identify parameters that may be either inputs or
    outputs from an OMX function at the same time.  This designation will 
    also be used in the case of a pointer that  points to a parameter that 
    is used both as an input and an output. */
#ifndef OMX_INOUT
#define OMX_INOUT
#endif

#define OMX_ALL 0xFFFFFFFF

typedef unsigned char OMX_U8;

typedef signed char OMX_S8;

typedef unsigned short OMX_U16;

typedef signed short OMX_S16;

typedef unsigned long OMX_U32;

typedef signed long OMX_S32;


/* Users with compilers that cannot accept the "long long" designation should
   define the OMX_SKIP64BIT macro.  It should be noted that this may cause 
   some components to fail to compile if the component was written to require
   64 bit integral types.  However, these components would NOT compile anyway
   since the compiler does not support the way the component was written.
*/
#ifndef OMX_SKIP64BIT
#ifdef __SYMBIAN32__

typedef unsigned long long OMX_U64;
typedef signed long long OMX_S64;

#elif defined(WIN32)

typedef unsigned __int64  OMX_U64;
typedef signed   __int64  OMX_S64;

#else /* WIN32 */

typedef unsigned long long OMX_U64;
typedef signed long long OMX_S64;

#endif /* WIN32 */
#endif

typedef enum OMX_BOOL {
    OMX_FALSE = 0,
    OMX_TRUE = !OMX_FALSE,
    OMX_BOOL_MAX = 0x7FFFFFFF
} OMX_BOOL; 
 
typedef void* OMX_PTR;

typedef char* OMX_STRING;

typedef unsigned char OMX_UUIDTYPE[128];

typedef enum OMX_DIRTYPE
{
    OMX_DirInput,
    OMX_DirOutput,
    OMX_DirMax = 0x7FFFFFFF
} OMX_DIRTYPE;

typedef enum OMX_ENDIANTYPE
{
    OMX_EndianBig,
    OMX_EndianLittle,
    OMX_EndianMax = 0x7FFFFFFF
} OMX_ENDIANTYPE;

typedef enum OMX_NUMERICALDATATYPE
{
    OMX_NumericalDataSigned,
    OMX_NumericalDataUnsigned,
    OMX_NumercialDataMax = 0x7FFFFFFF
} OMX_NUMERICALDATATYPE;


typedef struct OMX_BU32 {
    OMX_U32 nValue;
    OMX_U32 nMin;
    OMX_U32 nMax;
} OMX_BU32;


typedef struct OMX_BS32 {
    OMX_S32 nValue;
    OMX_S32 nMin;
    OMX_S32 nMax;
} OMX_BS32;


#ifndef OMX_SKIP64BIT
typedef OMX_S64 OMX_TICKS;
#else
typedef struct OMX_TICKS
{
    OMX_U32 nLowPart;
    OMX_U32 nHighPart;
} OMX_TICKS;
#endif
#define OMX_TICKS_PER_SECOND 1000000

/** Define the public interface for the OMX Handle.  The core will not use
    this value internally, but the application should only use this value.
 */
typedef void* OMX_HANDLETYPE;

typedef struct OMX_MARKTYPE
{
    OMX_HANDLETYPE hMarkTargetComponent;   /**< The component that will 
                                                generate a mark event upon 
                                                processing the mark. */
    OMX_PTR pMarkData;   /**< Application specific data associated with 
                              the mark sent on a mark event to disambiguate 
                              this mark from others. */
} OMX_MARKTYPE;


/** OMX_NATIVE_DEVICETYPE is used to map a OMX video port to the
 *  platform & operating specific object used to reference the display 
 *  or can be used by a audio port for native audio rendering */
typedef void* OMX_NATIVE_DEVICETYPE;

/** OMX_NATIVE_WINDOWTYPE is used to map a OMX video port to the
 *  platform & operating specific object used to reference the window */
typedef void* OMX_NATIVE_WINDOWTYPE;


/** Define the OMX IL version that corresponds to this set of header files.
 *  We also define a combined version that can be used to write or compare
 *  values of the 32bit nVersion field, assuming a little endian architecture */
#define OMX_VERSION_MAJOR 1
#define OMX_VERSION_MINOR 2
#define OMX_VERSION_REVISION 0
#define OMX_VERSION_STEP 0

#define OMX_VERSION ((OMX_VERSION_STEP<<24) | (OMX_VERSION_REVISION<<16) | (OMX_VERSION_MINOR<<8) | OMX_VERSION_MAJOR)


/** The OMX_VERSIONTYPE union is used to specify the version for
    a structure or component.  For a component, the version is entirely
    specified by the component vendor.  Components doing the same function
    from different vendors may or may not have the same version.  For 
    structures, the version shall be set by the entity that allocates the
    structure.  For structures specified in the OMX 1.2.0
    specification, the value of the version shall be set to
    1.2.0.0 in all cases.  Access to the OMX_VERSIONTYPE can be
    by a single 32 bit access (e.g. by nVersion) or by accessing
    one of the structure elements to, for example, check only
    the Major revision.
 */
typedef union OMX_VERSIONTYPE
{
    struct
    {
        OMX_U8 nVersionMajor;   /**< Major version accessor element */
        OMX_U8 nVersionMinor;   /**< Minor version accessor element */
        OMX_U8 nRevision;       /**< Revision version accessor element */
        OMX_U8 nStep;           /**< Step version accessor element */
    } s;
    OMX_U32 nVersion;           /**< 32 bit value to make accessing the
                                    version easily done in a single word
                                    size copy/compare operation */
} OMX_VERSIONTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
