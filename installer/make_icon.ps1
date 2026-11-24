param(
    [string]$Template = ""
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if ($Template -eq "") {
    $code = (Get-Command code.cmd -ErrorAction Stop).Source
    $install = Split-Path -Parent (Split-Path -Parent $code)
    $Template = (Get-ChildItem -LiteralPath $install -Recurse -Filter cpp.ico | Where-Object { $_.FullName -like "*resources\win32*" } | Select-Object -First 1).FullName
}
if (-not $Template -or -not (Test-Path -LiteralPath $Template)) {
    throw "VS Code's cpp.ico was not found; pass -Template <path to cpp.ico>"
}
$output = Join-Path $PSScriptRoot "rz.ico"
$orange = [System.Drawing.Color]::FromArgb(255, 234, 88, 12)

function Read-Frames([string]$path) {
    $data = [System.IO.File]::ReadAllBytes($path)
    $count = [BitConverter]::ToUInt16($data, 4)
    for ($index = 0; $index -lt $count; $index++) {
        $entry = 6 + 16 * $index
        $size = [int]$data[$entry]
        if ($size -eq 0) { $size = 256 }
        $length = [BitConverter]::ToInt32($data, $entry + 8)
        $offset = [BitConverter]::ToInt32($data, $entry + 12)
        if ($data[$offset] -eq 0x89) {
            $bytes = New-Object byte[] $length
            [Array]::Copy($data, $offset, $bytes, 0, $length)
            $image = [System.Drawing.Image]::FromStream((New-Object System.IO.MemoryStream(, $bytes)))
            $bitmap = New-Object System.Drawing.Bitmap($image)
        } else {
            $bitmap = (New-Object System.Drawing.Icon($path, $size, $size)).ToBitmap()
        }
        , @($size, $bitmap)
    }
}

function Get-Saturation([System.Drawing.Color]$pixel) {
    [Math]::Max($pixel.R, [Math]::Max($pixel.G, $pixel.B)) - [Math]::Min($pixel.R, [Math]::Min($pixel.G, $pixel.B))
}

function Test-Logo([System.Drawing.Color]$pixel, [int]$x, [int]$y, [int]$size) {
    if ($pixel.A -lt 40) { return $false }
    if ($x -gt $size * 0.55 -and $y -gt $size * 0.6) { return $false }
    return (Get-Saturation $pixel) -gt 24
}

function Get-PageColor([System.Drawing.Bitmap]$bitmap, [int]$size) {
    $counts = @{}
    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if ($pixel.A -eq 255 -and $pixel.R -gt 200 -and (Get-Saturation $pixel) -lt 8) {
                $counts[$pixel.ToArgb()] = 1 + [int]$counts[$pixel.ToArgb()]
            }
        }
    }
    $best = ($counts.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 1).Key
    [System.Drawing.Color]::FromArgb([int]$best)
}

function Convert-Frame([int]$size, [System.Drawing.Bitmap]$bitmap) {
    $left = $size; $top = $size; $right = -1; $bottom = -1
    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            if (Test-Logo $bitmap.GetPixel($x, $y) $x $y $size) {
                $left = [Math]::Min($left, $x); $right = [Math]::Max($right, $x)
                $top = [Math]::Min($top, $y); $bottom = [Math]::Max($bottom, $y)
            }
        }
    }
    if ($right -lt 0) { throw "no logo found in the $size pixel frame" }
    $page = Get-PageColor $bitmap $size
    $margin = [Math]::Max(1, [int]($size / 64))
    for ($y = [Math]::Max(0, $top - $margin); $y -le [Math]::Min($size - 1, $bottom + $margin); $y++) {
        for ($x = [Math]::Max(0, $left - $margin); $x -le [Math]::Min($size - 1, $right + $margin); $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            if ($pixel.A -gt 0 -and (Get-Saturation $pixel) -gt 6) {
                $bitmap.SetPixel($x, $y, $page)
            }
        }
    }
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias
    $box = New-Object System.Drawing.RectangleF($left, $top, ($right - $left + 1), ($bottom - $top + 1))
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $family = New-Object System.Drawing.FontFamily("Segoe UI")
    $path.AddString("rz", $family, [int][System.Drawing.FontStyle]::Bold, 100, (New-Object System.Drawing.PointF(0, 0)), [System.Drawing.StringFormat]::GenericTypographic)
    $bounds = $path.GetBounds()
    $scale = [Math]::Min($box.Width / $bounds.Width, $box.Height / $bounds.Height)
    $matrix = New-Object System.Drawing.Drawing2D.Matrix
    $matrix.Translate(($box.X + ($box.Width - $bounds.Width * $scale) / 2), ($box.Y + ($box.Height - $bounds.Height * $scale) / 2))
    $matrix.Scale($scale, $scale)
    $matrix.Translate(-$bounds.X, -$bounds.Y)
    $path.Transform($matrix)
    $graphics.FillPath((New-Object System.Drawing.SolidBrush $orange), $path)
    $graphics.Dispose()
    $stream = New-Object System.IO.MemoryStream
    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    , $stream.ToArray()
}

$frames = @(Read-Frames $Template)
$images = @($frames | ForEach-Object { , (Convert-Frame $_[0] $_[1]) })
$file = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($file)
$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$frames.Count)
$offset = 6 + 16 * $frames.Count
for ($index = 0; $index -lt $frames.Count; $index++) {
    $size = $frames[$index][0]
    $writer.Write([byte]($size % 256))
    $writer.Write([byte]($size % 256))
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]$images[$index].Length)
    $writer.Write([UInt32]$offset)
    $offset += $images[$index].Length
}
foreach ($image in $images) {
    $writer.Write($image)
}
$writer.Flush()
[System.IO.File]::WriteAllBytes($output, $file.ToArray())
$output
