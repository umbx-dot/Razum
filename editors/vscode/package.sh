#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
compiler="${1:?usage: package.sh RZC OUTPUT_DIRECTORY}"
output="${2:?usage: package.sh RZC OUTPUT_DIRECTORY}"
command -v zip > /dev/null || { echo "package.sh: zip is missing" >&2; exit 1; }
field() {
    python3 -c "import json, sys; print(json.load(open(sys.argv[1]))[sys.argv[2]])" "$here/package.json" "$1"
}
name="$(field name)"
version="$(field version)"
publisher="$(field publisher)"
display="$(field displayName)"
description="$(field description)"
engine="$(python3 -c "import json, sys; print(json.load(open(sys.argv[1]))['engines']['vscode'])" "$here/package.json")"
staging="$output/vscode-package"
rm -rf "$staging"
mkdir -p "$staging/extension/bin"
cp -r "$here/package.json" "$here/language-configuration.json" "$here/extension.js" "$here/icons" "$here/syntaxes" "$here/snippets" "$staging/extension/"
cp "$compiler" "$staging/extension/bin/rzc"
cp "$here/../../LICENSE" "$staging/extension/LICENSE.txt"
cp "$here/../../docs/linux-gui-notices.md" "$staging/extension/linux-gui-NOTICES.txt"
cat > "$staging/[Content_Types].xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".json" ContentType="application/json"/>
  <Default Extension=".js" ContentType="application/javascript"/>
  <Default Extension=".svg" ContentType="image/svg+xml"/>
  <Default Extension=".txt" ContentType="text/plain"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
  <Default Extension="" ContentType="application/octet-stream"/>
</Types>
XML
cat > "$staging/extension.vsixmanifest" <<XML
<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="$name" Version="$version" Publisher="$publisher"/>
    <DisplayName>$display</DisplayName>
    <Description xml:space="preserve">$description</Description>
    <Categories>Programming Languages</Categories>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="$engine"/>
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
XML
vsix="$output/$name-$version.vsix"
rm -f "$vsix"
(cd "$staging" && zip -q -r -X "$vsix" .)
echo "$vsix"
