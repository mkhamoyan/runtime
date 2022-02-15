// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using Internal.Cryptography;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace System.Security.Cryptography
{
    //
    // If you change anything in this class, you must make the same change in the other *Provider classes. This is a pain but given that the
    // preexisting contract from the .NET Framework locks all of these into deriving directly from the abstract HashAlgorithm class,
    // it can't be helped.
    //

    public abstract class SHA384 : HashAlgorithm
    {
        /// <summary>
        /// The hash size produced by the SHA384 algorithm, in bits.
        /// </summary>
        public const int HashSizeInBits = 384;

        /// <summary>
        /// The hash size produced by the SHA384 algorithm, in bytes.
        /// </summary>
        public const int HashSizeInBytes = HashSizeInBits / 8;

        protected SHA384()
        {
            HashSizeValue = HashSizeInBits;
        }

        public static new SHA384 Create() => new Implementation();

        [RequiresUnreferencedCode(CryptoConfig.CreateFromNameUnreferencedCodeMessage)]
        public static new SHA384? Create(string hashName) => (SHA384?)CryptoConfig.CreateFromName(hashName);

        /// <summary>
        /// Computes the hash of data using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The data to hash.</param>
        /// <returns>The hash of the data.</returns>
        /// <exception cref="ArgumentNullException">
        /// <paramref name="source" /> is <see langword="null" />.
        /// </exception>
        public static byte[] HashData(byte[] source!!)
        {
            return HashData(new ReadOnlySpan<byte>(source));
        }

        /// <summary>
        /// Computes the hash of data using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The data to hash.</param>
        /// <returns>The hash of the data.</returns>
        public static byte[] HashData(ReadOnlySpan<byte> source)
        {
            byte[] buffer = GC.AllocateUninitializedArray<byte>(HashSizeInBytes);

            int written = HashData(source, buffer.AsSpan());
            Debug.Assert(written == buffer.Length);

            return buffer;
        }

        /// <summary>
        /// Computes the hash of data using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The data to hash.</param>
        /// <param name="destination">The buffer to receive the hash value.</param>
        /// <returns>The total number of bytes written to <paramref name="destination" />.</returns>
        /// <exception cref="ArgumentException">
        /// The buffer in <paramref name="destination"/> is too small to hold the calculated hash
        /// size. The SHA384 algorithm always produces a 384-bit hash, or 48 bytes.
        /// </exception>
        public static int HashData(ReadOnlySpan<byte> source, Span<byte> destination)
        {
            if (!TryHashData(source, destination, out int bytesWritten))
                throw new ArgumentException(SR.Argument_DestinationTooShort, nameof(destination));

            return bytesWritten;
        }

        /// <summary>
        /// Attempts to compute the hash of data using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The data to hash.</param>
        /// <param name="destination">The buffer to receive the hash value.</param>
        /// <param name="bytesWritten">
        /// When this method returns, the total number of bytes written into <paramref name="destination"/>.
        /// </param>
        /// <returns>
        /// <see langword="false"/> if <paramref name="destination"/> is too small to hold the
        /// calculated hash, <see langword="true"/> otherwise.
        /// </returns>
        public static bool TryHashData(ReadOnlySpan<byte> source, Span<byte> destination, out int bytesWritten)
        {
            if (destination.Length < HashSizeInBytes)
            {
                bytesWritten = 0;
                return false;
            }

            bytesWritten = HashProviderDispenser.OneShotHashProvider.HashData(HashAlgorithmNames.SHA384, source, destination);
            Debug.Assert(bytesWritten == HashSizeInBytes);

            return true;
        }

        /// <summary>
        /// Computes the hash of a stream using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The stream to hash.</param>
        /// <param name="destination">The buffer to receive the hash value.</param>
        /// <returns>The total number of bytes written to <paramref name="destination" />.</returns>
        /// <exception cref="ArgumentNullException">
        ///   <paramref name="source" /> is <see langword="null" />.
        /// </exception>
        /// <exception cref="ArgumentException">
        ///   <p>
        ///   The buffer in <paramref name="destination"/> is too small to hold the calculated hash
        ///   size. The SHA384 algorithm always produces a 384-bit hash, or 48 bytes.
        ///   </p>
        ///   <p>-or-</p>
        ///   <p>
        ///   <paramref name="source" /> does not support reading.
        ///   </p>
        /// </exception>
        public static int HashData(Stream source!!, Span<byte> destination)
        {
            if (destination.Length < HashSizeInBytes)
                throw new ArgumentException(SR.Argument_DestinationTooShort, nameof(destination));

            if (!source.CanRead)
                throw new ArgumentException(SR.Argument_StreamNotReadable, nameof(source));

            return LiteHashProvider.HashStream(HashAlgorithmNames.SHA384, HashSizeInBytes, source, destination);
        }

        /// <summary>
        /// Computes the hash of a stream using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The stream to hash.</param>
        /// <returns>The hash of the data.</returns>
        /// <exception cref="ArgumentNullException">
        ///   <paramref name="source" /> is <see langword="null" />.
        /// </exception>
        /// <exception cref="ArgumentException">
        ///   <paramref name="source" /> does not support reading.
        /// </exception>
        public static byte[] HashData(Stream source!!)
        {
            if (!source.CanRead)
                throw new ArgumentException(SR.Argument_StreamNotReadable, nameof(source));

            return LiteHashProvider.HashStream(HashAlgorithmNames.SHA384, HashSizeInBytes, source);
        }

        /// <summary>
        /// Asynchronously computes the hash of a stream using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The stream to hash.</param>
        /// <param name="cancellationToken">
        ///   The token to monitor for cancellation requests.
        ///   The default value is <see cref="System.Threading.CancellationToken.None" />.
        /// </param>
        /// <returns>The hash of the data.</returns>
        /// <exception cref="ArgumentNullException">
        ///   <paramref name="source" /> is <see langword="null" />.
        /// </exception>
        /// <exception cref="ArgumentException">
        ///   <paramref name="source" /> does not support reading.
        /// </exception>
        public static ValueTask<byte[]> HashDataAsync(Stream source!!, CancellationToken cancellationToken = default)
        {
            if (!source.CanRead)
                throw new ArgumentException(SR.Argument_StreamNotReadable, nameof(source));

            return LiteHashProvider.HashStreamAsync(HashAlgorithmNames.SHA384, HashSizeInBytes, source, cancellationToken);
        }

        /// <summary>
        /// Asynchronously computes the hash of a stream using the SHA384 algorithm.
        /// </summary>
        /// <param name="source">The stream to hash.</param>
        /// <param name="destination">The buffer to receive the hash value.</param>
        /// <param name="cancellationToken">
        ///   The token to monitor for cancellation requests.
        ///   The default value is <see cref="System.Threading.CancellationToken.None" />.
        /// </param>
        /// <returns>The total number of bytes written to <paramref name="destination" />.</returns>
        /// <exception cref="ArgumentNullException">
        ///   <paramref name="source" /> is <see langword="null" />.
        /// </exception>
        /// <exception cref="ArgumentException">
        ///   <p>
        ///   The buffer in <paramref name="destination"/> is too small to hold the calculated hash
        ///   size. The SHA384 algorithm always produces a 384-bit hash, or 48 bytes.
        ///   </p>
        ///   <p>-or-</p>
        ///   <p>
        ///   <paramref name="source" /> does not support reading.
        ///   </p>
        /// </exception>
        public static ValueTask<int> HashDataAsync(
            Stream source!!,
            Memory<byte> destination,
            CancellationToken cancellationToken = default)
        {
            if (destination.Length < HashSizeInBytes)
                throw new ArgumentException(SR.Argument_DestinationTooShort, nameof(destination));

            if (!source.CanRead)
                throw new ArgumentException(SR.Argument_StreamNotReadable, nameof(source));

            return LiteHashProvider.HashStreamAsync(
                HashAlgorithmNames.SHA384,
                HashSizeInBytes,
                source,
                destination,
                cancellationToken);
        }

        private sealed class Implementation : SHA384
        {
            private readonly HashProvider _hashProvider;

            public Implementation()
            {
                _hashProvider = HashProviderDispenser.CreateHashProvider(HashAlgorithmNames.SHA384);
                HashSizeValue = _hashProvider.HashSizeInBytes * 8;
            }

            protected sealed override void HashCore(byte[] array, int ibStart, int cbSize) =>
                _hashProvider.AppendHashData(array, ibStart, cbSize);

            protected sealed override void HashCore(ReadOnlySpan<byte> source) =>
                _hashProvider.AppendHashData(source);

            protected sealed override byte[] HashFinal() =>
                _hashProvider.FinalizeHashAndReset();

            protected sealed override bool TryHashFinal(Span<byte> destination, out int bytesWritten) =>
                _hashProvider.TryFinalizeHashAndReset(destination, out bytesWritten);

            public sealed override void Initialize() => _hashProvider.Reset();

            protected sealed override void Dispose(bool disposing)
            {
                _hashProvider.Dispose(disposing);
                base.Dispose(disposing);
            }
        }
    }
}
