@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================================
echo   Lab 3 (MPI) - auto setup, build and run
echo ============================================================

cd /d "%~dp0"

REM ============================================================
REM 1. Find MS-MPI runtime (mpiexec)
REM ============================================================
set "MPIEXEC="
if exist "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" set "MPIEXEC=C:\Program Files\Microsoft MPI\Bin\mpiexec.exe"
where mpiexec.exe >nul 2>&1
if not errorlevel 1 if "!MPIEXEC!"=="" set "MPIEXEC=mpiexec.exe"

REM ============================================================
REM 2. Find MS-MPI SDK (mpi.h + msmpi.lib)
REM ============================================================
set "MPI_INC="
set "MPI_LIB="
if exist "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h" (
    set "MPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
)
if exist "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\msmpi.lib" (
    set "MPI_LIB=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"
)

REM ============================================================
REM 3. Find MSVC (vcvars64.bat)
REM ============================================================
set "VCVARS="
for %%E in (Community Professional Enterprise BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
    )
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvars64.bat"
    )
)

REM ============================================================
REM 4. If anything is missing - elevate and install
REM ============================================================
set "NEED_INSTALL=0"
if "!MPIEXEC!"==""  set "NEED_INSTALL=1"
if "!MPI_INC!"==""  set "NEED_INSTALL=1"
if "!MPI_LIB!"==""  set "NEED_INSTALL=1"
if "!VCVARS!"==""   set "NEED_INSTALL=1"

if "!NEED_INSTALL!"=="1" (
    net session >nul 2>&1
    if errorlevel 1 (
        echo.
        echo [!] Some dependencies are missing ^(MS-MPI / MS-MPI SDK / MSVC^).
        echo [!] Administrator rights are required to install them.
        echo [!] Re-launching as Administrator...
        powershell -NoProfile -Command "Start-Process -Verb RunAs -FilePath '%~f0' -WorkingDirectory '%~dp0'"
        exit /b
    )

    if "!MPIEXEC!"=="" (
        echo.
        echo [1/6] Installing MS-MPI runtime via winget...
        winget install --id Microsoft.MPI --silent --accept-source-agreements --accept-package-agreements
        if exist "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" (
            set "MPIEXEC=C:\Program Files\Microsoft MPI\Bin\mpiexec.exe"
        )
    )

    if "!MPI_INC!"=="" (
        echo.
        echo [1/6] Installing MS-MPI SDK via winget...
        winget install --id Microsoft.MPI.SDK --silent --accept-source-agreements --accept-package-agreements
        if exist "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h" (
            set "MPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
        )
        if exist "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\msmpi.lib" (
            set "MPI_LIB=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"
        )
    )

    if "!VCVARS!"=="" (
        echo.
        echo [2/6] Installing VS 2022 Build Tools (C++ workload) via winget...
        winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-source-agreements --accept-package-agreements --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
        set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
)

if "!MPIEXEC!"=="" (
    echo [!] mpiexec not found. Install MS-MPI runtime from
    echo     https://www.microsoft.com/en-us/download/details.aspx?id=105289
    goto :END
)
if "!MPI_INC!"=="" (
    echo [!] mpi.h not found. Install MS-MPI SDK from
    echo     https://www.microsoft.com/en-us/download/details.aspx?id=105289
    goto :END
)
if not exist "!VCVARS!" (
    echo [!] vcvars64.bat not found: !VCVARS!
    goto :END
)

echo [1/6] MS-MPI runtime: !MPIEXEC!
echo [1/6] MS-MPI SDK include: !MPI_INC!
echo [1/6] MS-MPI SDK lib:     !MPI_LIB!
echo [2/6] MSVC: !VCVARS!

REM Activate MSVC environment
call "!VCVARS!" >nul

if not exist matrix mkdir matrix
if not exist result mkdir result

REM ============================================================
REM 5. Generate input matrices if missing
REM ============================================================
if not exist matrix\first_2000.txt (
    echo.
    echo [3/6] Generating input matrices...
    set PYTHONIOENCODING=utf-8
    python generate_all_matrices.py
    if errorlevel 1 (
        echo [!] Generation failed. Install Python and numpy first: pip install numpy
        goto :END
    )
) else (
    echo [3/6] Input matrices already exist
)

REM ============================================================
REM 6. Compile main.cpp with MPI
REM ============================================================
echo.
echo [4/6] Compiling main.cpp (MS-MPI)...
cl /nologo /O2 /EHsc /std:c++17 /I "!MPI_INC!" main.cpp /Fe:main.exe /link /LIBPATH:"!MPI_LIB!" msmpi.lib
if errorlevel 1 (
    echo [!] Compilation failed.
    goto :END
)
del /q main.obj 2>nul
echo [4/6] Build OK: main.exe

REM ============================================================
REM 7. Clear stats and run for 1, 2, 4, 8 processes
REM ============================================================
if exist result\statistics_mpi.txt del /q result\statistics_mpi.txt

echo.
echo [5/6] Running experiments...
echo ------------------------------------------------------------
for %%P in (1 2 4 8) do (
    echo --- MPI processes: %%P ---
    "!MPIEXEC!" -n %%P main.exe
    if errorlevel 1 (
        echo [!] MPI run failed for %%P processes.
        goto :END
    )
    echo.
)
echo ------------------------------------------------------------

REM ============================================================
REM 8. Verify and plot
REM ============================================================
set PYTHONIOENCODING=utf-8

if exist verify.py (
    echo.
    echo [6/6] Verification via numpy...
    python verify.py
)

if exist plot_results.py (
    echo.
    echo Generating plot...
    python plot_results.py
)

echo.
echo ============================================================
echo   Done
echo ============================================================

:END
echo.
pause
