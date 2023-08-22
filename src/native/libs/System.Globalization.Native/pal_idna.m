// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
//

#include <stdint.h>

#include "pal_icushim_internal.h"
#include "pal_idna.h"
#include "pal_errors.h"
#import <Foundation/Foundation.h>
#import <System.Globalization.Native-Swift.h>

#if defined(TARGET_MACCATALYST) || defined(TARGET_IOS) || defined(TARGET_TVOS)
// static const uint32_t AllowUnassigned = 0x1;
// static const uint32_t UseStd3AsciiRules = 0x2;

// static uint32_t GetOptions(uint32_t flags, uint32_t useToAsciiFlags)
// {
//     uint32_t options = UIDNA_CHECK_CONTEXTJ;

//     if ((flags & AllowUnassigned) == AllowUnassigned)
//     {
//         options |= UIDNA_ALLOW_UNASSIGNED;
//     }

//     if ((flags & UseStd3AsciiRules) == UseStd3AsciiRules)
//     {
//         options |= UIDNA_USE_STD3_RULES;
//     }

//     if (useToAsciiFlags)
//     {
//         options |=  UIDNA_NONTRANSITIONAL_TO_ASCII;
//     }
//     else
//     {
//         options |=  UIDNA_NONTRANSITIONAL_TO_UNICODE;
//     }

//     return options;
// }

/*
Function:
ToASCII

Used by System.Globalization.IdnMapping.GetAsciiCore to convert an Unicode
domain name to ASCII

Return values:
0: internal error during conversion.
>0: the length of the converted string (not including the null terminator).
*/
int32_t GlobalizationNative_ToAsciiNative(
    uint32_t flags, const uint16_t* lpSrc, int32_t cwSrcLength, uint16_t* lpDst, int32_t cwDstLength)
{
   /* UErrorCode err = U_ZERO_ERROR;
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;

    UIDNA* pIdna = uidna_openUTS46(GetOptions(flags, 1), &err);

    int32_t asciiStrLen = uidna_nameToASCII(pIdna, lpSrc, cwSrcLength, lpDst, cwDstLength, &info, &err);

    // To have a consistent behavior with Windows, we mask out the error when having 2 hyphens in the third and fourth place.
    info.errors &= (uint32_t)~UIDNA_ERROR_HYPHEN_3_4;

    uidna_close(pIdna);

    return ((U_SUCCESS(err) || (err == U_BUFFER_OVERFLOW_ERROR)) && (info.errors == 0)) ? asciiStrLen : 0;*/
    /*NSString* srcString = [NSString stringWithCString: lpSrc encoding:NSUnicodeStringEncoding];
        
    NSData *decode = [srcString dataUsingEncoding:NSNonLossyASCIIStringEncoding];
    NSString *asciiString = [[NSString alloc] initWithData:decode encoding:NSNonLossyASCIIStringEncoding];

    if (asciiString == NULL || asciiString.length == 0)
    {
        return 0;
    }*/
    
    NSString *sourceString = [NSString stringWithCharacters: lpSrc length: cwSrcLength];
    NSString *idnEncoded = sourceString.idnaEncoded;
    int32_t index = 0, dstIdx = 0, isError = 0;
    uint16_t dstCodepoint;
    while (index < idnEncoded.length)
    {
        dstCodepoint = [idnEncoded characterAtIndex: index];
        Append(lpDst, dstIdx, cwDstLength, dstCodepoint, isError);
        index++;
    }

    return !isError ? [idnEncoded length] : 0;
}

/*
Function:
ToUnicode

Used by System.Globalization.IdnMapping.GetUnicodeCore to convert an ASCII name
to Unicode

Return values:
0: internal error during conversion.
>0: the length of the converted string (not including the null terminator).
*/
int32_t GlobalizationNative_ToUnicodeNative(
    uint32_t flags, const uint16_t* lpSrc, int32_t cwSrcLength, uint16_t* lpDst, int32_t cwDstLength)
{
    NSString *sourceString = [NSString stringWithCharacters: lpSource length: cwSrcLength];
    NSString *idnaDecoded = sourceString.idnaDecoded;
    int32_t index = 0, dstIdx = 0, isError = 0;
    uint16_t dstCodepoint;
    while (index < idnaDecoded.length)
    {
        dstCodepoint = [idnaDecoded characterAtIndex: index];
        Append(lpDst, dstIdx, cwDstLength, dstCodepoint, isError);
        index++;
    }

    return !isError ? [idnaDecoded length] : 0;
}
#endif