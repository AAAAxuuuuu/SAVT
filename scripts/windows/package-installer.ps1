[CmdletBinding()]
param(
    [string]$QtRoot,
    [string]$BuildDir,
    [string]$StageDir,
    [string]$OutputDir,
    [string]$Version,
    [switch]$IncludeLocalAiConfig,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Resolve-RepoRoot {
    $scriptRoot = $PSScriptRoot
    return (Resolve-Path (Join-Path $scriptRoot '..\..')).Path
}

function Resolve-QtRoot {
    param([string]$ExplicitQtRoot)

    if ($ExplicitQtRoot) {
        return (Resolve-Path $ExplicitQtRoot).Path
    }

    if ($env:SAVT_QT_ROOT) {
        return (Resolve-Path $env:SAVT_QT_ROOT).Path
    }

    $candidates = @(
        'F:\QT\6.9.3\msvc2022_64',
        'C:\Qt\6.9.3\msvc2022_64'
    )

    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate 'bin\windeployqt.exe')) {
            return (Resolve-Path $candidate).Path
        }
    }

    $globRoots = @('F:\QT', 'C:\Qt')
    foreach ($globRoot in $globRoots) {
        if (-not (Test-Path $globRoot)) {
            continue
        }

        $match = Get-ChildItem -Path $globRoot -Directory |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'msvc2022_64' } |
            Where-Object { Test-Path (Join-Path $_ 'bin\windeployqt.exe') } |
            Select-Object -First 1
        if ($match) {
            return (Resolve-Path $match).Path
        }
    }

    throw 'Could not resolve Qt MSVC root. Pass -QtRoot or set SAVT_QT_ROOT.'
}

function Resolve-IsccPath {
    $candidates = @(
        'F:\Inno Setup 6\ISCC.exe',
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $registryLocations = @(
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1',
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1'
    )

    foreach ($registryLocation in $registryLocations) {
        if (-not (Test-Path $registryLocation)) {
            continue
        }

        $installLocation = (Get-ItemProperty -Path $registryLocation -ErrorAction Stop).InstallLocation
        if ($installLocation) {
            $candidate = Join-Path $installLocation 'ISCC.exe'
            if (Test-Path $candidate) {
                return (Resolve-Path $candidate).Path
            }
        }
    }

    throw 'Could not find ISCC.exe. Install Inno Setup 6 first.'
}

function Resolve-CMakePath {
    $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -eq 0 -and $vsInstall) {
            $candidate = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (Test-Path $candidate) {
                return (Resolve-Path $candidate).Path
            }
        }
    }

    $fallbacks = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Preview\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    )

    foreach ($candidate in $fallbacks) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw 'Could not find cmake.exe.'
}

function Resolve-NinjaRoot {
    $command = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if ($command) {
        return Split-Path -Parent $command.Source
    }

    $cmakePath = Resolve-CMakePath
    $candidate = Join-Path (Split-Path -Parent (Split-Path -Parent $cmakePath)) 'Ninja\ninja.exe'
    if (Test-Path $candidate) {
        return Split-Path -Parent $candidate
    }

    $fallbacks = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Preview\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
    )

    foreach ($candidateDir in $fallbacks) {
        if (Test-Path (Join-Path $candidateDir 'ninja.exe')) {
            return $candidateDir
        }
    }

    throw 'Could not find ninja.exe.'
}

function Resolve-AppVersion {
    param(
        [string]$ExplicitVersion,
        [string]$RepoRoot
    )

    if ($ExplicitVersion) {
        return $ExplicitVersion
    }

    $cmakeText = Get-Content -Path (Join-Path $RepoRoot 'CMakeLists.txt') -Raw
    $match = [regex]::Match($cmakeText, 'project\s*\(\s*SAVT\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', 'IgnoreCase')
    if (-not $match.Success) {
        throw 'Could not parse project version from CMakeLists.txt.'
    }

    return $match.Groups[1].Value
}

function Invoke-BatchFile {
    param(
        [string]$VcvarsPath,
        [string]$QtRootPath,
        [string]$NinjaRootPath,
        [string[]]$Commands
    )

    $tempFile = Join-Path $env:TEMP ("savt-package-{0}.cmd" -f ([guid]::NewGuid().ToString('N')))
    $lines = @(
        '@echo off',
        'setlocal',
        ('call "{0}" >nul' -f $VcvarsPath),
        'if errorlevel 1 exit /b 1'
        ('set "PATH={0};{1}\bin;%PATH%"' -f $NinjaRootPath, $QtRootPath),
        ('set "CMAKE_PREFIX_PATH={0}"' -f $QtRootPath),
        ('set "Qt6_DIR={0}\lib\cmake\Qt6"' -f $QtRootPath)
    ) + $Commands
    Set-Content -Path $tempFile -Value $lines -Encoding ASCII

    try {
        & cmd.exe /c $tempFile
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed in batch wrapper: $tempFile"
        }
    }
    finally {
        Remove-Item -Path $tempFile -Force -ErrorAction SilentlyContinue
    }
}

function Resolve-VcvarsPath {
    if ($env:SAVT_VCVARS_PATH -and (Test-Path $env:SAVT_VCVARS_PATH)) {
        return (Resolve-Path $env:SAVT_VCVARS_PATH).Path
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -eq 0 -and $vsInstall) {
            $candidate = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars64.bat'
            if (Test-Path $candidate) {
                return (Resolve-Path $candidate).Path
            }
        }
    }

    $fallbacks = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
    )

    foreach ($candidate in $fallbacks) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw 'Could not find vcvars64.bat.'
}

function Resolve-MsvcRuntimeDir {
    param([string]$VcvarsPath)

    $vcRoot = Resolve-Path (Join-Path (Split-Path -Parent (Split-Path -Parent $VcvarsPath)) '..')
    $redistRoot = Join-Path $vcRoot 'Redist\MSVC'
    if (-not (Test-Path $redistRoot)) {
        throw "Could not find MSVC redist root under $redistRoot"
    }

    $candidate = Get-ChildItem -Path $redistRoot -Directory |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName 'x64\Microsoft.VC143.CRT' } |
        Where-Object { Test-Path (Join-Path $_ 'vcruntime140.dll') } |
        Select-Object -First 1

    if (-not $candidate) {
        throw 'Could not locate the x64 Microsoft.VC143.CRT redist directory.'
    }

    return (Resolve-Path $candidate).Path
}

$repoRoot = Resolve-RepoRoot
$qtRootPath = Resolve-QtRoot -ExplicitQtRoot $QtRoot
$vcvarsPath = Resolve-VcvarsPath
$msvcRuntimeDir = Resolve-MsvcRuntimeDir -VcvarsPath $vcvarsPath
$cmakePath = Resolve-CMakePath
$ninjaRootPath = Resolve-NinjaRoot
$ninjaExePath = Join-Path $ninjaRootPath 'ninja.exe'
$isccPath = Resolve-IsccPath
$windeployqtPath = Join-Path $qtRootPath 'bin\windeployqt.exe'
$versionValue = Resolve-AppVersion -ExplicitVersion $Version -RepoRoot $repoRoot

if (-not (Test-Path $windeployqtPath)) {
    throw "windeployqt.exe not found under $qtRootPath"
}

$resolvedBuildDir = if ($BuildDir) { (Resolve-Path $BuildDir).Path } else { Join-Path $repoRoot 'build\windows-msvc-qt-release' }
$resolvedStageDir = if ($StageDir) { $StageDir } else { Join-Path $repoRoot 'output\windows-installer\stage' }
$resolvedOutputDir = if ($OutputDir) { $OutputDir } else { Join-Path $repoRoot 'output\windows-installer\dist' }
$issPath = Join-Path $repoRoot 'packaging\windows\SAVT.iss'
$appExe = Join-Path $resolvedBuildDir 'savt_desktop.exe'
$localAiConfigPath = Join-Path $repoRoot 'config\deepseek-ai.local.json'

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if (-not $SkipBuild) {
    Write-Step 'Configuring Release build'
    Invoke-BatchFile -VcvarsPath $vcvarsPath -QtRootPath $qtRootPath -NinjaRootPath $ninjaRootPath -Commands @(
        ('"{0}" -S "{1}" -B "{2}" -G Ninja --fresh -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="{3}" -DCMAKE_MAKE_PROGRAM="{4}" -DSAVT_BUILD_TESTS=OFF -DSAVT_ENABLE_CLANG_TOOLING=OFF' -f $cmakePath, $repoRoot, $resolvedBuildDir, $qtRootPath, $ninjaExePath)
    )

    Write-Step 'Building savt_desktop Release target'
    Invoke-BatchFile -VcvarsPath $vcvarsPath -QtRootPath $qtRootPath -NinjaRootPath $ninjaRootPath -Commands @(
        ('"{0}" --build "{1}" --target savt_desktop --parallel' -f $cmakePath, $resolvedBuildDir)
    )
}

if (-not (Test-Path $appExe)) {
    throw "Release executable not found: $appExe"
}

Write-Step 'Preparing staging directory'
if (Test-Path $resolvedStageDir) {
    Remove-Item -Path $resolvedStageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $resolvedStageDir -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $resolvedStageDir 'config') -Force | Out-Null

Copy-Item -Path $appExe -Destination (Join-Path $resolvedStageDir 'savt_desktop.exe') -Force
Copy-Item -Path (Join-Path $repoRoot 'config\deepseek-ai.template.json') -Destination (Join-Path $resolvedStageDir 'config\deepseek-ai.template.json') -Force
if ($IncludeLocalAiConfig) {
    if (-not (Test-Path $localAiConfigPath)) {
        throw "Requested -IncludeLocalAiConfig, but no local AI config was found at $localAiConfigPath"
    }

    Copy-Item -Path $localAiConfigPath -Destination (Join-Path $resolvedStageDir 'config\deepseek-ai.local.json') -Force
    Write-Step 'Included local AI config in installer payload'
}
Copy-Item -Path (Join-Path $msvcRuntimeDir '*.dll') -Destination $resolvedStageDir -Force

Write-Step 'Deploying Qt runtime with windeployqt'
& $windeployqtPath `
    --release `
    --compiler-runtime `
    --qmldir (Join-Path $repoRoot 'apps\desktop\qml') `
    --dir $resolvedStageDir `
    (Join-Path $resolvedStageDir 'savt_desktop.exe')
if ($LASTEXITCODE -ne 0) {
    throw 'windeployqt failed.'
}

Write-Step 'Compiling Chinese Inno Setup installer'
& $isccPath `
    ("/DAppVersion={0}" -f $versionValue) `
    ("/DSourceRoot={0}" -f $repoRoot) `
    ("/DStageDir={0}" -f $resolvedStageDir) `
    ("/DOutputDir={0}" -f $resolvedOutputDir) `
    $issPath
if ($LASTEXITCODE -ne 0) {
    throw 'ISCC.exe failed.'
}

$installerPath = Join-Path $resolvedOutputDir ("SAVT-{0}-zh-CN-setup.exe" -f $versionValue)
if (-not (Test-Path $installerPath)) {
    throw "Expected installer output was not found: $installerPath"
}

Write-Step 'Packaging complete'
Write-Host "Installer: $installerPath" -ForegroundColor Green
