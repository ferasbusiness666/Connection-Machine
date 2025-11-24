set(FUZZING_DIR "${CMAKE_SOURCE_DIR}/fuzzing")

file(GLOB_RECURSE PROJECT_SOURCES
	"${SOURCE_DIR}/*.cpp"
)
string(REGEX REPLACE "([][+.*^$(){}|\\\\])" "\\\\\\1" SOURCE_DIR_REGEX "${SOURCE_DIR}")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "${SOURCE_DIR_REGEX}/cli/main.cpp$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "${SOURCE_DIR_REGEX}/main.cpp$")

file(GLOB_RECURSE FUZZING_SOURCES
	"${FUZZING_DIR}/*.cpp"
)

add_executable(${PROJECT_NAME}_fuzzer ${PROJECT_SOURCES} ${FUZZING_SOURCES})

add_main_dependencies()

target_include_directories(${PROJECT_NAME}_fuzzer PRIVATE ${FUZZING_DIR} ${SOURCE_DIR})
target_link_libraries(${PROJECT_NAME}_fuzzer PRIVATE ${EXTERNAL_LINKS})
target_precompile_headers(${PROJECT_NAME}_fuzzer PRIVATE "${SOURCE_DIR}/precompiled.h")

target_compile_definitions(${PROJECT_NAME}_fuzzer PRIVATE VK_NO_PROTOTYPES)
target_compile_definitions(${PROJECT_NAME}_fuzzer PRIVATE "PROJECT_VERSION=\"${PROJECT_VERSION}\"")
target_compile_definitions(${PROJECT_NAME}_fuzzer PRIVATE "FUZZER")
