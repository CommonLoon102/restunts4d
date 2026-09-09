using System.Buffers.Binary;
using System.Globalization;
using System.Text;

namespace DumpSrv;

internal static class DumpOutput
{
    private const int ReplayHeaderSize = 26;
    private const int RecordedFramesOffset = 24;
    private const int PhysicsHeaderSize = 2;
    private const int GameStateSize = 1120;
    private const int RendererSampleInterval = 5;
    private const int DigestLength = 32;

    public static async Task<ushort> ReadFrameCountAsync(string replayPath,
        CancellationToken cancellationToken)
    {
        await using var stream = File.OpenRead(replayPath);
        var header = new byte[ReplayHeaderSize];
        var count = await stream.ReadAtLeastAsync(header, header.Length, false, cancellationToken);
        if (count != header.Length)
        {
            throw new InvalidDataException(
                $"Incomplete replay header: {Path.GetFileName(replayPath)}");
        }
        return BinaryPrimitives.ReadUInt16LittleEndian(header.AsSpan(RecordedFramesOffset));
    }

    public static async Task<bool> IsCompleteAsync(string path, bool renderer, ushort frames,
        CancellationToken cancellationToken)
    {
        if (!File.Exists(path))
        {
            return false;
        }
        await using var stream = File.OpenRead(path);
        if (renderer)
        {
            return await IsRendererCompleteAsync(stream, frames, cancellationToken);
        }
        // An interrupted DOS write can leave an empty file, a partial state, or
        // even a whole number of states. Require the replay's entire timeline.
        if (stream.Length != PhysicsHeaderSize + (long)frames * GameStateSize)
        {
            return false;
        }
        var header = new byte[PhysicsHeaderSize];
        await stream.ReadExactlyAsync(header, cancellationToken);
        return BinaryPrimitives.ReadUInt16LittleEndian(header) == frames;
    }

    private static async Task<bool> IsRendererCompleteAsync(FileStream stream, ushort frames,
        CancellationToken cancellationToken)
    {
        using var reader = new StreamReader(stream, Encoding.ASCII, false, leaveOpen: true);
        long expectedBytes = 0;
        for (var frame = 0; frame <= frames; frame += RendererSampleInterval)
        {
            var prefix = frame.ToString(CultureInfo.InvariantCulture) + " ";
            var line = await reader.ReadLineAsync(cancellationToken);
            if (line is null || line.Length != prefix.Length + DigestLength ||
                !line.StartsWith(prefix, StringComparison.Ordinal) ||
                line.Skip(prefix.Length).Any(character =>
                    character is not (>= '0' and <= '9') and not (>= 'a' and <= 'f')))
            {
                return false;
            }
            expectedBytes += line.Length + 2; // Every DOS sample ends with CRLF.
        }
        return stream.Length == expectedBytes &&
            await reader.ReadLineAsync(cancellationToken) is null;
    }
}
