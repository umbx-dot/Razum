param([int]$Port = 8765)

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$built = Join-Path $here "..\build\rzc.exe"
$installed = Get-Command rzc -ErrorAction SilentlyContinue
$rzc = if (Test-Path $built) { (Resolve-Path $built).Path } elseif ($installed) { $installed.Source } else { $built }
if (-not (Test-Path $rzc)) {
    Write-Error "rzc was not found: install razum (razum-setup.exe) or build it into build\rzc.exe."
    exit 1
}
$server = Join-Path $here "razum-learn.exe"
$running = Get-Process -Name "razum-learn" -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -eq $server }
if ($running) {
    Write-Error "This copy of the Razum site is already running. Use its open browser tab, or stop it with Ctrl+C in the terminal that started it and run this script again."
    exit 1
}
& $rzc (Join-Path $here "learn.rz") -o $server
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Start-Process "http://localhost:$Port"
& $server --port $Port --rzc $rzc
