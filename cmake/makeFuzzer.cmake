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

# Copy resources next to the fuzzer executable so DirectoryManager can find them
set(FUZZER_RESOURCES_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")
file(GLOB_RECURSE FUZZER_RESOURCE_FILES RELATIVE "${RESOURCES_DIR}" "${RESOURCES_DIR}/*")
set(FUZZER_RESOURCE_PRODUCTS)
foreach(resource_path_relative IN LISTS FUZZER_RESOURCE_FILES)
	set(original_resource "${RESOURCES_DIR}/${resource_path_relative}")
	set(copied_resource "${FUZZER_RESOURCES_BINARY_DIR}/${resource_path_relative}")
	get_filename_component(copied_resource_dir "${copied_resource}" DIRECTORY)

	add_custom_command(
		OUTPUT "${copied_resource}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${copied_resource_dir}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${original_resource}" "${copied_resource}"
		DEPENDS "${original_resource}"
		COMMENT "Copying resource for fuzzer: ${resource_path_relative}"
		VERBATIM
	)

	list(APPEND FUZZER_RESOURCE_PRODUCTS "${copied_resource}")
endforeach()

set(FUZZER_CIRCUITS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/fuzzing/circuits")
set(FUZZER_CIRCUIT_PRODUCTS)
if(EXISTS "${FUZZER_CIRCUITS_SOURCE_DIR}")
	file(GLOB_RECURSE FUZZER_CIRCUIT_FILES RELATIVE "${FUZZER_CIRCUITS_SOURCE_DIR}" "${FUZZER_CIRCUITS_SOURCE_DIR}/*")
	foreach(circuit_path_relative IN LISTS FUZZER_CIRCUIT_FILES)
		set(original_circuit "${FUZZER_CIRCUITS_SOURCE_DIR}/${circuit_path_relative}")
		set(copied_circuit "${FUZZER_RESOURCES_BINARY_DIR}/circuits/${circuit_path_relative}")
		get_filename_component(copied_circuit_dir "${copied_circuit}" DIRECTORY)

		add_custom_command(
			OUTPUT "${copied_circuit}"
			COMMAND "${CMAKE_COMMAND}" -E make_directory "${copied_circuit_dir}"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${original_circuit}" "${copied_circuit}"
			DEPENDS "${original_circuit}"
			COMMENT "Copying fuzzer circuit: ${circuit_path_relative}"
			VERBATIM
		)

		list(APPEND FUZZER_CIRCUIT_PRODUCTS "${copied_circuit}")
	endforeach()
else()
	message(STATUS "Fuzzer circuits directory not found: ${FUZZER_CIRCUITS_SOURCE_DIR}")
endif()

add_custom_target(copy-fuzzer-resources DEPENDS ${FUZZER_RESOURCE_PRODUCTS} ${FUZZER_CIRCUIT_PRODUCTS})
add_dependencies(${PROJECT_NAME}_fuzzer copy-fuzzer-resources)
