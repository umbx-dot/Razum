param(
    [switch]$Test,
    [switch]$Installer,
    [switch]$Install,
    [switch]$Help
)

$ErrorActionPreference = 'Stop'

if ($Help) {
    @'
Usage: .\build.ps1 [-Test] [-Installer] [-Install]

Builds rzc.exe into .\build (or $env:RZ_BUILD) with CMake.

  -Test        run the test suite afterwards (tests/run_all.sh, needs Git Bash)
  -Installer   also build build\installer\razum-setup.exe
  -Install     build the installer and install razum for this user (no administrator rights)

Needs a C17 compiler (GCC from MSYS2 UCRT64, or MSVC from a Developer PowerShell),
CMake 3.20+, Ninja and NASM on PATH. See README.md, "Build from source".
'@
    exit 0
}

$root = $PSScriptRoot
$build = if ($env:RZ_BUILD) { $env:RZ_BUILD } else { Join-Path $root 'build' }

$missing = @()
foreach ($tool in @('cmake', 'nasm', 'ninja')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { $missing += $tool }
}
if (-not (Get-Command gcc -ErrorAction SilentlyContinue) -and -not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $missing += 'gcc or cl'
}
if ($missing.Count -gt 0) {
    Write-Error ("build.ps1: missing: " + ($missing -join ', ') + ". Run .\build.ps1 -Help for what to install.")
}

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    $ErrorActionPreference = 'Continue'
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path -LiteralPath (Join-Path $build 'CMakeCache.txt'))) {
    Invoke-Checked cmake @('-S', $root, '-B', $build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
}
Invoke-Checked cmake @('--build', $build)
Write-Host "built $(Join-Path $build 'rzc.exe')"

if ($Test) {
    $bash = Get-Command bash -ErrorAction SilentlyContinue
    if (-not $bash) { throw 'build.ps1: -Test needs Git Bash (bash) on PATH' }
    $env:RZ_BUILD = $build
    Invoke-Checked $bash.Source @((Join-Path $root 'tests/run_all.sh'))
}
if ($Installer -or $Install) {
    Invoke-Checked cmake @('--build', $build, '--target', 'razum-setup')
    Write-Host "built $(Join-Path $build 'installer\razum-setup.exe')"
}
if ($Install) {
    $setup = Join-Path $build 'installer\razum-setup.exe'
    $process = Start-Process -FilePath $setup -ArgumentList '--quiet' -Wait -PassThru -NoNewWindow
    if ($process.ExitCode -ne 0) { throw "razum-setup.exe failed with exit code $($process.ExitCode)" }
    Write-Host 'installed; open a new terminal and run: rzc --help'
}
