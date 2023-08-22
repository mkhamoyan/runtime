// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
//

#pragma once

#include "pal_locale.h"
#include "pal_compiler.h"

PALEXPORT int32_t GlobalizationNative_ToAscii(uint32_t flags,
                                              const UChar* lpSrc,
                                              int32_t cwSrcLength,
                                              UChar* lpDst,
                                              int32_t cwDstLength);

PALEXPORT int32_t GlobalizationNative_ToUnicode(uint32_t flags,
                                                const UChar* lpSrc,
                                                int32_t cwSrcLength,
                                                UChar* lpDst,
                                                int32_t cwDstLength);
#ifdef __APPLE__
PALEXPORT int32_t GlobalizationNative_ToAsciiNative(uint32_t flags,
                                              const uint16_t* lpSrc,
                                              int32_t cwSrcLength,
                                              uint16_t* lpDst,
                                              int32_t cwDstLength);

PALEXPORT int32_t GlobalizationNative_ToUnicodeNative(uint32_t flags,
                                                const uint16_t* lpSrc,
                                                int32_t cwSrcLength,
                                                uint16_t* lpDst,
                                                int32_t cwDstLength);
#endif