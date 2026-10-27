using System.Text;
using System.Text.Json;

namespace Runefall.Build;

internal static class RuntimeJson
{
    public static void WriteAll(string dir)
    {
        foreach (var csv in Directory.GetFiles(dir, "*.csv"))
        {
            var table = Path.GetFileName(csv);
            var lines = File.ReadAllLines(csv, Encoding.UTF8);
            if (lines.Length == 0)
                continue;
            var columns = ParseLine(lines[0]);
            var rows = new List<Dictionary<string, string>>();
            for (var i = 1; i < lines.Length; ++i)
            {
                if (lines[i].Length == 0)
                    continue;
                var cells = ParseLine(lines[i]);
                var row = new Dictionary<string, string>(StringComparer.Ordinal);
                for (var c = 0; c < columns.Count; ++c)
                    row[columns[c]] = c < cells.Count ? cells[c] : "";
                rows.Add(row);
            }
            var payload = new
            {
                schemaVersion = 1,
                table,
                columns,
                rows
            };
            var json = JsonSerializer.Serialize(payload, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(Path.ChangeExtension(csv, ".json"), json + "\n", new UTF8Encoding(false));
        }
    }

    public static void RemoveCsv(string dir)
    {
        foreach (var csv in Directory.GetFiles(dir, "*.csv"))
            File.Delete(csv);
    }

    private static List<string> ParseLine(string line)
    {
        var cells = new List<string>();
        var current = new StringBuilder();
        var quoted = false;
        for (var i = 0; i < line.Length; ++i)
        {
            var c = line[i];
            if (quoted)
            {
                if (c == '"' && i + 1 < line.Length && line[i + 1] == '"')
                {
                    current.Append('"');
                    ++i;
                }
                else if (c == '"')
                {
                    quoted = false;
                }
                else
                {
                    current.Append(c);
                }
            }
            else if (c == '"')
            {
                quoted = true;
            }
            else if (c == ',')
            {
                cells.Add(current.ToString());
                current.Clear();
            }
            else
            {
                current.Append(c);
            }
        }
        cells.Add(current.ToString());
        return cells;
    }
}
