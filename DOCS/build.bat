@echo off
echo Building AppGuard documentation...

set /p choice="Install documentation requirements? (y/n): "
if /i "%choice%"=="y" (
    echo Installing requirements...
    pip install -r requirements.txt
) else (
    echo Skipped requirements installation
)

echo.
echo Running Doxygen to generate C++ API documentation...
doxygen Doxyfile

echo.
echo Building Sphinx documentation...
cd source
sphinx-build -b html . _build

echo.
echo Documentation built successfully!
echo Open _build/index.html to view the documentation.

pause