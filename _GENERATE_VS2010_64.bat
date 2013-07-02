@echo on

mkdir build
chdir build

del CMakeCache.txt

cmake -G "Visual Studio 10 Win64" ../

if %errorlevel% NEQ 0 goto error
goto end

:error
echo Houve um erro. Pressione qualquer tecla para finalizar.
pause >nul

:end
