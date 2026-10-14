param([switch]$MathOnly)
$ErrorActionPreference = 'Stop'
$repository = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repository 'posibleProyecto'
$build = Join-Path $repository 'x64/shadow-tests'
New-Item -ItemType Directory -Force -Path $build | Out-Null

# Encuentra el compilador instalado e importa su entorno en este proceso.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Instala Visual Studio con Desarrollo para el escritorio con C++.' }
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'No se encontro una instalacion de Visual C++.' }
$devCommand = Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
$environmentLines = & $env:ComSpec /d /s /c "`"$devCommand`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'No se pudo preparar el entorno de Visual C++.' }
foreach ($line in $environmentLines) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}

$sources = @('Renderer.cpp', 'ShadowMap.cpp', 'shaders.cpp', 'Mesh.cpp', 'MeshLibrary.cpp', 'cubo.cpp', 'suelo.cpp') |
    ForEach-Object { Join-Path $project $_ }
$arguments = @('/nologo', '/EHsc', '/std:c++20', '/W4', '/O2', '/MD', '/DGLEW_STATIC',
    "/I$project", "/I$project/libs/glm-master", "/I$project/libs/glew-2.3.1/include",
    "/I$project/libs/glfw-3.5.1/include", "/Fe$build/shadow_tests.exe", "/Fo$build/",
    (Join-Path $PSScriptRoot 'shadow_tests.cpp')) + $sources + @('/link', '/DEBUG:NONE', '/INCREMENTAL:NO',
    "/LIBPATH:$project/libs/glew-2.3.1/lib/Release/x64", "/LIBPATH:$project/libs/glfw-3.5.1/lib-vc2026",
    'opengl32.lib', 'glew32s.lib', 'glfw3.lib', 'user32.lib', 'gdi32.lib', 'shell32.lib')
& cl.exe @arguments
if ($LASTEXITCODE -ne 0) { throw 'No se pudieron compilar las pruebas.' }

$testArguments = @('--capture-dir', $build)
if ($MathOnly) { $testArguments = @('--math-only') }
& (Join-Path $build 'shadow_tests.exe') @testArguments
if ($LASTEXITCODE -ne 0) { throw 'Las pruebas han fallado.' }
