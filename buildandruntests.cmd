cmake --build --preset tests
if %ERRORLEVEL% == 0 (
    rm -r coverage
    call .\opencpp_coverage.cmd
)