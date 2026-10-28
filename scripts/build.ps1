param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Generator = '',
    [string]$Compiler = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $projectRoot "build-$Configuration"
$configureArgs = @('-S', $projectRoot, '-B', $buildDir, "-DCMAKE_BUILD_TYPE=$Configuration")
if ($Generator) { $configureArgs += @('-G', $Generator) }
if ($Compiler) { $configureArgs += "-DCMAKE_CXX_COMPILER=$Compiler" }
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake 配置失败' }
& cmake --build $buildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw '构建失败' }
& ctest --test-dir $buildDir -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '测试失败' }
