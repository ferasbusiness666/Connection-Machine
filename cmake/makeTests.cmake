if(CONNECTION_MACHINE_CODE_COVERAGE)
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		message(STATUS "Code coverage enabled")
		add_compile_options(--coverage -O0 -g)
		add_link_options(--coverage)
	elseif(MSVC)
		message(WARNING "Code coverage does nothing with MSVC. Use normal Tests build, and use opencpp_coverage.cmd")
	else()
		message(WARNING "Code coverage not working with: \"${CMAKE_CXX_COMPILER_ID}\"")
	endif()
endif()


# Google Testing
CPMAddPackage(
	NAME gtest
	GITHUB_REPOSITORY google/googletest
	GIT_TAG v1.17.0
	SOURCE_DIR "${EXTERNAL_DIR}/gtest"
	OPTIONS "gtest_force_shared_crt ON"
)

set(TEST_DIR "${CMAKE_SOURCE_DIR}/tests")
set(PROJECT_SOURCES)
file(GLOB_RECURSE PROJECT_SOURCES
	"${SOURCE_DIR}/*.cpp"
	"${TEST_DIR}/*.cpp"
)
string(REGEX REPLACE "([][+.*^$(){}|\\\\])" "\\\\\\1" SOURCE_DIR_REGEX "${SOURCE_DIR}")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "${SOURCE_DIR_REGEX}/cli/main.cpp$")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "${SOURCE_DIR_REGEX}\/main.cpp")

add_main_dependencies()

set(EXTERNAL_LINKS ${EXTERNAL_LINKS} gtest gtest_main)

message("EXTERNAL_LINKS: ${EXTERNAL_LINKS}")

if(APPLE)
	# Link CoreFoundation explicitly on macOS
	list(APPEND EXTERNAL_LINKS "-framework CoreFoundation")
endif()

add_executable(${PROJECT_NAME}_tests ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}_tests PRIVATE ${SOURCE_DIR} ${TEST_DIR} "${EXTERNAL_DIR}/wasmtime")
target_compile_definitions(${PROJECT_NAME}_tests PRIVATE VK_NO_PROTOTYPES)
target_compile_definitions(${PROJECT_NAME}_tests PRIVATE "PROJECT_VERSION=\"${PROJECT_VERSION}\"")

target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${EXTERNAL_LINKS})

target_precompile_headers(${PROJECT_NAME}_tests PRIVATE "${SOURCE_DIR}/precompiled.h")

set(TEST_RESOURCES_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")

file(GLOB_RECURSE TEST_RESOURCE_FILES RELATIVE "${RESOURCES_DIR}" "${RESOURCES_DIR}/*")
set(TEST_RESOURCE_PRODUCTS)
foreach(resource_path_relative IN LISTS TEST_RESOURCE_FILES)
	set(original_resource "${RESOURCES_DIR}/${resource_path_relative}")
	set(copied_resource "${TEST_RESOURCES_BINARY_DIR}/${resource_path_relative}")
	get_filename_component(copied_resource_dir "${copied_resource}" DIRECTORY)

	add_custom_command(
		OUTPUT "${copied_resource}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${copied_resource_dir}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${original_resource}" "${copied_resource}"
		DEPENDS "${original_resource}"
		COMMENT "Copying resource for tests: ${resource_path_relative}"
		VERBATIM
	)

	list(APPEND TEST_RESOURCE_PRODUCTS "${copied_resource}")
endforeach()

set(TEST_CIRCUITS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/tests/circuits")
set(TEST_CIRCUIT_PRODUCTS)
if(EXISTS "${TEST_CIRCUITS_SOURCE_DIR}")
	file(GLOB_RECURSE TEST_CIRCUIT_FILES RELATIVE "${TEST_CIRCUITS_SOURCE_DIR}" "${TEST_CIRCUITS_SOURCE_DIR}/*")
	foreach(circuit_path_relative IN LISTS TEST_CIRCUIT_FILES)
		set(original_circuit "${TEST_CIRCUITS_SOURCE_DIR}/${circuit_path_relative}")
		set(copied_circuit "${TEST_RESOURCES_BINARY_DIR}/circuits/${circuit_path_relative}")
		get_filename_component(copied_circuit_dir "${copied_circuit}" DIRECTORY)

		add_custom_command(
			OUTPUT "${copied_circuit}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${copied_circuit_dir}"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${original_circuit}" "${copied_circuit}"
			DEPENDS "${original_circuit}"
			COMMENT "Copying test circuit: ${circuit_path_relative}"
			VERBATIM
		)

		list(APPEND TEST_CIRCUIT_PRODUCTS "${copied_circuit}")
	endforeach()
else()
	message(WARNING "Test circuits directory not found: ${TEST_CIRCUITS_SOURCE_DIR}")
endif()

add_custom_target(copy-test-resources DEPENDS ${TEST_RESOURCE_PRODUCTS} ${TEST_CIRCUIT_PRODUCTS})
add_dependencies(${PROJECT_NAME}_tests copy-test-resources)
