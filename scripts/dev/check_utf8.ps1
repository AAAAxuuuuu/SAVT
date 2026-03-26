param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedRoot = (Resolve-Path $Root).Path
$utf8Strict = [System.Text.UTF8Encoding]::new($false, $true)

$includedExtensions = @(
    ".c", ".cc", ".cpp", ".cxx",
    ".h", ".hh", ".hpp", ".hxx",
    ".qml", ".qrc",
    ".cmake",
    ".md", ".txt",
    ".json", ".toml", ".xml", ".yml", ".yaml",
    ".js", ".ts", ".tsx", ".css", ".html",
    ".py", ".java", ".ps1"
)

$includedFileNames = @(
    "CMakeLists.txt",
    ".editorconfig",
    ".gitignore"
)

$excludedPrefixes = @(
    ".git/",
    "build/",
    "third_party/",
    "tools/downloads/",
    "tools/llvm/"
)

$excludedFilePatterns = @(
    "^moc_.*\.cpp$",
    "^qrc_.*\.cpp$",
    "^ui_.*\.h$",
    ".*\.qmlc$",
    ".*\.bak$",
    ".*\.user$"
)

function Get-RelativePath([string]$BasePath, [string]$Path) {
    $baseUri = [System.Uri](([System.IO.Path]::GetFullPath($BasePath).TrimEnd('\', '/')) +
        [System.IO.Path]::DirectorySeparatorChar)
    $pathUri = [System.Uri][System.IO.Path]::GetFullPath($Path)
    return [System.Uri]::UnescapeDataString($baseUri.MakeRelativeUri($pathUri).ToString()).Replace('\', '/')
}

function Should-ScanFile([System.IO.FileInfo]$FileInfo) {
    $relativePath = Get-RelativePath -BasePath $resolvedRoot -Path $FileInfo.FullName
    foreach ($prefix in $excludedPrefixes) {
        if ($relativePath.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $false
        }
    }

    foreach ($pattern in $excludedFilePatterns) {
        if ($FileInfo.Name -match $pattern) {
            return $false
        }
    }

    if ($includedFileNames -contains $FileInfo.Name) {
        return $true
    }

    return $includedExtensions -contains $FileInfo.Extension.ToLowerInvariant()
}

function Test-Utf8Bytes([byte[]]$Bytes) {
    try {
        [void]$utf8Strict.GetString($Bytes)
        return $true
    } catch [System.ArgumentException] {
        return $false
    }
}

function Format-BytePreview([byte[]]$Bytes) {
    $previewLength = [Math]::Min($Bytes.Length, 12)
    if ($previewLength -eq 0) {
        return "<empty>"
    }

    $preview = for ($index = 0; $index -lt $previewLength; ++$index) {
        $Bytes[$index].ToString("X2")
    }
    return ($preview -join " ")
}

$invalidFiles = New-Object System.Collections.Generic.List[object]

Get-ChildItem -Path $resolvedRoot -Recurse -File | ForEach-Object {
    if (-not (Should-ScanFile -FileInfo $_)) {
        return
    }

    $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
    if (Test-Utf8Bytes -Bytes $bytes) {
        return
    }

    $invalidFiles.Add([PSCustomObject]@{
        RelativePath = Get-RelativePath -BasePath $resolvedRoot -Path $_.FullName
        BytePreview = Format-BytePreview -Bytes $bytes
    })
}

if ($invalidFiles.Count -gt 0) {
    Write-Host "Found non-UTF-8 text files:" -ForegroundColor Red
    foreach ($file in $invalidFiles) {
        Write-Host (" - {0} | bytes: {1}" -f $file.RelativePath, $file.BytePreview)
    }
    exit 1
}

Write-Host ("UTF-8 check passed for source files under {0}" -f $resolvedRoot) -ForegroundColor Green
