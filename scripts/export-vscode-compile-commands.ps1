param(
    [Parameter(Mandatory = $true)]
    [string]$SourcePath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

function Convert-MsysToken {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Token
    )

    $quote = ""
    if ($Token.Length -ge 2 -and $Token.StartsWith('"') -and $Token.EndsWith('"')) {
        $quote = '"'
        $Token = $Token.Substring(1, $Token.Length - 2)
    }

    $Token = $Token -replace '^/([A-Za-z])/', '$1:/'
    $Token = $Token -replace '^-I/([A-Za-z])/', '-I$1:/'
    $Token = $Token -replace '^-L/([A-Za-z])/', '-L$1:/'
    $Token = $Token -replace '(?<=\=)/([A-Za-z])/', '$1:/'
    $Token = $Token -replace '(?<=;)/([A-Za-z])/', '$1:/'

    if ($quote) {
        return $quote + $Token + $quote
    }

    return $Token
}

function Split-CommandLine {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CommandLine
    )

    $matches = [regex]::Matches($CommandLine, '"(?:\\.|[^"])*"|\S+')
    $tokens = foreach ($match in $matches) {
        $match.Value
    }
    return ,$tokens
}

if (-not (Test-Path $SourcePath)) {
    throw "Missing compile_commands.json at $SourcePath"
}

$entries = Get-Content $SourcePath -Raw | ConvertFrom-Json

foreach ($entry in $entries) {
    foreach ($field in @("directory", "file", "output")) {
        if ($entry.PSObject.Properties.Name -contains $field -and $entry.$field) {
            $entry.$field = Convert-MsysToken -Token $entry.$field
        }
    }

    if ($entry.PSObject.Properties.Name -contains "command" -and $entry.command) {
        $tokens = Split-CommandLine -CommandLine $entry.command
        $entry.command = (($tokens | ForEach-Object {
                    Convert-MsysToken -Token $_
                }) -join ' ')
    }
}

$outputDir = Split-Path -Parent $OutputPath
if ($outputDir) {
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
}

$entries | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
