// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Diagnostics;
using System.Text;

namespace System.Globalization
{
    internal static partial class Normalization
    {
        internal static bool IsNormalized(string strInput, NormalizationForm normalizationForm)
        {
            if (GlobalizationMode.Invariant)
            {
                // In Invariant mode we assume all characters are normalized.
                // This is because we don't support any linguistic operation on the strings
                return true;
            }

            return GlobalizationMode.UseNls ?
                NlsIsNormalized(strInput, normalizationForm) :
// #if TARGET_MACCATALYST || TARGET_IOS || TARGET_TVOS
//                 GlobalizationMode.Hybrid ?
//                     NativeIsNormalized(strInput, normalizationForm) :
//                     IcuIsNormalized(strInput, normalizationForm);
// #else
                IcuIsNormalized(strInput, normalizationForm);
// #endif
        }

        internal static string Normalize(string strInput, NormalizationForm normalizationForm)
        {
            if (GlobalizationMode.Invariant)
            {
                // In Invariant mode we assume all characters are normalized.
                // This is because we don't support any linguistic operation on the strings
                return strInput;
            }

            return GlobalizationMode.UseNls ?
                NlsNormalize(strInput, normalizationForm) :
// #if TARGET_MACCATALYST || TARGET_IOS || TARGET_TVOS
//                 GlobalizationMode.Hybrid ?
//                     NativeNormalize(strInput, normalizationForm) :
//                     IcuNormalize(strInput, normalizationForm);
// #else
                IcuNormalize(strInput, normalizationForm);
// #endif
        }
    }
}
