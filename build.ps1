# Requires PowerShell 5.1+
# Purpose: agentic, structured build automation

function Run-Command {
    param(
        [string]$Command,
        [string]$ErrorMessage
    )

    Write-Host "▶️  $Command"
    Invoke-Expression $Command
    if ($LASTEXITCODE -ne 0) {
        throw "❌ $ErrorMessage (exit code: $LASTEXITCODE)"
    }
}

function Sync-Venv {
    Write-Host "🔄 Syncing environment..."
    Run-Command "uv sync" "uv sync failed"
    & .venv/Scripts/activate.ps1
}

function Export-Pkg {
    param([string]$BuildType)

    conan list "wgpu-native/27.0.2.0:*" --package-query="build_type=$BuildType" *> $null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Package exists in local cache"
    } else {
        Write-Host "📦 Exporting wgpu-native package for $BuildType"
        Run-Command "conan export-pkg external/wgpu-native --version=27.0.2.0 -s:a build_type=$BuildType" `
            "Conan export failed for $BuildType"
    }
}

function Install-Deps {
    param([string]$BuildType)
    Write-Host "📦 Installing dependencies for $BuildType"
    Run-Command "conan install raktr/ --output-folder=. -pr:a=profiles/msvc_vs.profile -o:a='&:with_tests=True' --build=missing -s:a=build_type=$BuildType" `
        "Conan install failed for $BuildType"
}

function Build-Project {
    Write-Host "🛠️ Building project..."
    Push-Location raktr
    try {
        Run-Command "cmake --preset conan-default -DBUILD_TESTS=ON" "CMake configuration failed"
        Run-Command "cmake --build --preset conan-release" "Build failed"
    }
    finally {
        Pop-Location
    }
}

function Main {
    $initialDir = Get-Location
    try {
        Sync-Venv
        Export-Pkg -BuildType Release
        Export-Pkg -BuildType Debug
        Install-Deps -BuildType Release
        Install-Deps -BuildType Debug
        Build-Project
        Write-Host "✅ All tasks completed successfully."
    }
    catch {
        Write-Host "🚨 ERROR: $($_.Exception.Message)"
        exit 1
    }
    finally {
        Set-Location -Path $initialDir
    }
}

Main
