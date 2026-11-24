param(
    [string]$Compiler = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "build\rzc.exe"),
    [string]$OutputDirectory = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "build")
)
$ErrorActionPreference = "Stop"

$manifest = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot "package.json") | ConvertFrom-Json
$compiler = $Compiler
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "rzc.exe not found at $compiler; build it first with: cmake --build build"
}

$staging = Join-Path $OutputDirectory "vscode-package"
if (Test-Path -LiteralPath $staging) {
    Remove-Item -LiteralPath $staging -Recurse -Force
}
$extension = Join-Path $staging "extension"
New-Item -ItemType Directory -Force -Path (Join-Path $extension "bin") | Out-Null
foreach ($item in "package.json", "language-configuration.json", "extension.js", "icons", "syntaxes", "snippets") {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $item) -Destination $extension -Recurse
}
Copy-Item -LiteralPath $compiler -Destination (Join-Path $extension "bin")

$contentTypes = @'
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".json" ContentType="application/json"/>
  <Default Extension=".js" ContentType="application/javascript"/>
  <Default Extension=".svg" ContentType="image/svg+xml"/>
  <Default Extension=".exe" ContentType="application/octet-stream"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
</Types>
'@
$vsixManifest = @"
<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="$($manifest.name)" Version="$($manifest.version)" Publisher="$($manifest.publisher)"/>
    <DisplayName>$($manifest.displayName)</DisplayName>
    <Description xml:space="preserve">$($manifest.description)</Description>
    <Categories>Programming Languages</Categories>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="$($manifest.engines.vscode)"/>
    </Properties>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code"/>
  </Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true"/>
  </Assets>
</PackageManifest>
"@
[System.IO.File]::WriteAllText((Join-Path $staging "[Content_Types].xml"), $contentTypes)
[System.IO.File]::WriteAllText((Join-Path $staging "extension.vsixmanifest"), $vsixManifest)

$vsix = Join-Path $OutputDirectory "$($manifest.name)-$($manifest.version).vsix"
if (Test-Path -LiteralPath $vsix) {
    Remove-Item -LiteralPath $vsix -Force
}
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($vsix, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in Get-ChildItem -LiteralPath $staging -Recurse -File) {
        $entryName = $file.FullName.Substring($staging.Length + 1).Replace("\", "/")
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entryName) | Out-Null
    }
} finally {
    $archive.Dispose()
}
$vsix
