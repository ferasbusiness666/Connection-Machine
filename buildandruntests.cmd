cmake --build --preset tests
if %ERRORLEVEL% == 0 (
    call "build-tests/Debug/Connection_Machine_tests.exe" --gtest_filter=*Fuzz*
)