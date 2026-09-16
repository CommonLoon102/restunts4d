namespace DumpSrv;

public static class ReplayCatalog
{
    public static IReadOnlyList<string> Discover(string directory, CancellationToken cancellationToken = default)
    {
        var names = new List<string>();
        var identities = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var path in Directory.EnumerateFiles(directory, "*", SearchOption.TopDirectoryOnly))
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (!Path.GetExtension(path).Equals(".rpl", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }
            var name = Path.GetFileName(path);
            ValidateBaseName(Path.GetFileNameWithoutExtension(name));
            if (!identities.Add(name))
            {
                throw new InvalidDataException($"Case-insensitive replay filename collision: {name}");
            }
            names.Add(name);
        }
        if (names.Count == 0)
        {
            throw new InvalidDataException($"No replay files found in {directory}.");
        }
        names.Sort(StringComparer.Ordinal);
        return names;
    }

    public static void ValidateBaseName(string name)
    {
        const string punctuation = "!#$'()@_`{}~-";
        if (name.Length is < 1 or > 8 || name.Any(character =>
            !char.IsAsciiLetterOrDigit(character) && !punctuation.Contains(character)))
        {
            throw new InvalidDataException($"Unsupported DOS 8.3 replay basename: {name}");
        }
        var upper = name.ToUpperInvariant();
        if (upper is "CON" or "PRN" or "AUX" or "NUL" or "CLOCK$" or "CONIN$" or "CONOUT$" ||
            (upper.Length == 4 && (upper.StartsWith("COM", StringComparison.Ordinal) ||
                upper.StartsWith("LPT", StringComparison.Ordinal)) && upper[3] is >= '1' and <= '9'))
        {
            throw new InvalidDataException($"Reserved DOS device replay basename: {name}");
        }
    }

    public static IReadOnlyList<string> RendererReplays(string directory,
        IReadOnlyList<string> replays, int target, CancellationToken cancellationToken = default)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(target, 0);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(target, 1);
        if (target == 0)
        {
            return replays;
        }
        const int opponentTypeOffset = 6;
        Span<byte> header = stackalloc byte[26];
        var opponents = new List<string>();
        foreach (var replay in replays)
        {
            cancellationToken.ThrowIfCancellationRequested();
            using var stream = File.OpenRead(Path.Combine(directory, replay));
            if (stream.ReadAtLeast(header, header.Length, false) != header.Length)
            {
                throw new InvalidDataException($"Incomplete replay header: {replay}");
            }
            if (header[opponentTypeOffset] != 0)
            {
                opponents.Add(replay);
            }
        }
        return opponents;
    }

    public static IReadOnlyList<string> Sample(IReadOnlyList<string> replays, int percentage)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(percentage, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(percentage, 100);
        var count = (int)(((long)replays.Count * percentage + 99) / 100);
        var sample = new List<string>(count);
        for (var index = 0; index < count; index++)
        {
            sample.Add(replays[(int)((long)index * replays.Count / count)]);
        }
        return sample;
    }

    public static List<List<T>> RoundRobin<T>(IReadOnlyList<T> items, int count)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(count, 1);
        var lists = Enumerable.Range(0, count).Select(_ => new List<T>()).ToList();
        var destination = 0;
        foreach (var item in items)
        {
            lists[destination].Add(item);
            destination = (destination + 1) % count;
        }
        return lists;
    }

}

public static class DosFiles
{
    public static string Resolve(string directory, string fileName)
    {
        // MatchCasing makes this work on both case-sensitive Unix hosts and Windows.
        var matches = Directory.EnumerateFiles(directory, fileName, new EnumerationOptions
        {
            MatchCasing = MatchCasing.CaseInsensitive,
            MatchType = MatchType.Simple,
            AttributesToSkip = 0
        }).Where(path => Path.GetFileName(path).Equals(fileName, StringComparison.OrdinalIgnoreCase))
            .Take(2).ToArray();
        return matches.Length switch
        {
            0 => Path.Combine(directory, fileName),
            1 => matches[0],
            _ => throw new InvalidDataException($"Case-insensitive DOS filename collision: {fileName}")
        };
    }
}
