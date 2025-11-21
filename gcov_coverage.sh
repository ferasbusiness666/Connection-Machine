mkdir -p coverage
gcovr \
    --verbose \
    --gcov-executable gcov-14 \
    --root . \
    --filter 'src/.*' \
    --exclude 'external/.*' \
    --exclude '/usr/include/.*' \
    --gcov-exclude-directories 'build/_deps' \
    --html \
    --html-nested \
    -o coverage/coverage.html \
    --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
    --gcov-ignore-errors=no_working_dir_found
echo "Find coverage report at: $(realpath ./coverage/coverage.html)"
