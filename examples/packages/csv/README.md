# Razum CSV

Read fields from a CSV record with `import csv;`. From a Razum project, install the package with `rz add umbx-dot/razum-csv`.

```c
import csv;
import stdio;

int main() {
    string row = "one,\"two,2\",three";
    int cursor = 0;
    char field[32];
    while (true) {
        int result = csv_field(row, &cursor, field);
        if (result < 0) { return 1; }
        print(field);
        if (result == 0) { return 0; }
    }
}
```

`csv_field` writes a NUL-terminated field and advances the byte cursor. Start at zero. It returns `1` if another field follows, `0` for the last field, or `-1` for malformed CSV, an invalid cursor, or a buffer too small for the field. It handles quoted commas, doubled quotes, and empty fields. Pass one complete record without its line terminator.

MIT licensed. [Razum](https://github.com/umbx-dot/Razum)
