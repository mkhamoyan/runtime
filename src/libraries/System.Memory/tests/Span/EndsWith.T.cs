// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using Xunit;

namespace System.SpanTests
{
    public static partial class SpanTests
    {
        [Fact]
        public static void ZeroLengthEndsWith()
        {
            int[] a = new int[3];

            Span<int> first = new Span<int>(a, 1, 0);
            ReadOnlySpan<int> second = new ReadOnlySpan<int>(a, 2, 0);
            bool b = first.EndsWith(second);
            Assert.True(b);
        }

        [Fact]
        public static void SameSpanEndsWith()
        {
            int[] a = { 4, 5, 6 };
            Span<int> span = new Span<int>(a);
            bool b = span.EndsWith(span);
            Assert.True(b);
        }

        [Fact]
        public static void LengthMismatchEndsWith()
        {
            int[] a = { 4, 5, 6 };
            Span<int> first = new Span<int>(a, 0, 2);
            ReadOnlySpan<int> second = new ReadOnlySpan<int>(a, 0, 3);
            bool b = first.EndsWith(second);
            Assert.False(b);
        }

        [Fact]
        public static void EndsWithMatch()
        {
            int[] a = { 4, 5, 6 };
            Span<int> span = new Span<int>(a, 0, 3);
            ReadOnlySpan<int> slice = new ReadOnlySpan<int>(a, 1, 2);
            bool b = span.EndsWith(slice);
            Assert.True(b);
        }

        [Fact]
        public static void EndsWithMatchDifferentSpans()
        {
            int[] a = { 4, 5, 6 };
            int[] b = { 4, 5, 6 };
            Span<int> span = new Span<int>(a, 0, 3);
            ReadOnlySpan<int> slice = new ReadOnlySpan<int>(b, 0, 3);
            bool c = span.EndsWith(slice);
            Assert.True(c);
        }

        [Fact]
        public static void OnEndsWithOfEqualSpansMakeSureEveryElementIsCompared()
        {
            for (int length = 0; length < 100; length++)
            {
                TIntLog log = new TIntLog();

                TInt[] first = new TInt[length];
                TInt[] second = new TInt[length];
                for (int i = 0; i < length; i++)
                {
                    first[i] = second[i] = new TInt(10 * (i + 1), log);
                }

                Span<TInt> firstSpan = new Span<TInt>(first);
                ReadOnlySpan<TInt> secondSpan = new ReadOnlySpan<TInt>(second);
                bool b = firstSpan.EndsWith(secondSpan);
                Assert.True(b);

                // Make sure each element of the array was compared once. (Strictly speaking, it would not be illegal for
                // EndsWith to compare an element more than once but that would be a non-optimal implementation and
                // a red flag. So we'll stick with the stricter test.)
                Assert.Equal(first.Length, log.Count);
                foreach (TInt elem in first)
                {
                    int numCompares = log.CountCompares(elem.Value, elem.Value);
                    Assert.True(numCompares == 1, $"Expected {numCompares} == 1 for element {elem.Value}.");
                }
            }
        }

        [Fact]
        public static void EndsWithSingle()
        {
            ReadOnlySpan<char> chars = [];
            Assert.False(chars.EndsWith('\0'));
            Assert.False(chars.EndsWith('f'));

            chars = "foo";
            Assert.True(chars.EndsWith(chars[^1]));
            Assert.True(chars.EndsWith('o'));
            Assert.False(chars.EndsWith('f'));

            scoped ReadOnlySpan<string> strings = [];
            Assert.False(strings.EndsWith((string)null));
            Assert.False(strings.EndsWith("foo"));

            strings = ["foo", "bar"];
            Assert.True(strings.EndsWith(strings[^1]));
            Assert.True(strings.EndsWith("bar"));
            Assert.True(strings.EndsWith("*bar".Substring(1)));
            Assert.False(strings.EndsWith("foo"));
            Assert.False(strings.EndsWith((string)null));

            strings = ["foo", null];
            Assert.True(strings.EndsWith(strings[^1]));
            Assert.True(strings.EndsWith((string)null));
            Assert.False(strings.EndsWith("foo"));
        }
    }
}
