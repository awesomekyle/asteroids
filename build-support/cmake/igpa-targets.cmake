# Set some global compile flags
set(ALL_C_FLAGS CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
set(ALL_CXX_FLAGS CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
if(MSVC AND NOT ${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    foreach(flag_var ${ALL_C_FLAGS} ${ALL_CXX_FLAGS})
        # Disable RTTI
        string(REPLACE "/GR" "/GR-" ${flag_var} "${${flag_var}}")

        # Disable exceptions
        string(REGEX REPLACE "/EH[a-z]+" "" ${flag_var} "${${flag_var}}")
    endforeach()
endif()
if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU OR ${CMAKE_CXX_COMPILER_ID} MATCHES Clang)

    # Some options need to be specified for only C or C++
    foreach(flag_var ${ALL_CXX_FLAGS})
        # -fno-rtti         Disable RTTI
        # -fno-exceptions   Disable exceptions
        set(${flag_var} "${${flag_var}} -std=c++11  -fno-rtti -fno-exceptions")
    endforeach()
    foreach(flag_var ${ALL_C_FLAGS})
        # -std=c99          MSVC only supports c99
        set(${flag_var} "${${flag_var}} -std=c99")
    endforeach()
endif()

# If the user didn't specify output directories from the command line, use these
# defaults here.
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY not specified; using ${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY not specified; using ${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

################################################################################
function(set_source_folder GENERATED FILE ABSOLUTE_NAME ABSOLUTE_PATH)
    file(RELATIVE_PATH RELATIVE_NAME ${ABSOLUTE_PATH} ${ABSOLUTE_NAME})
    get_filename_component(RELATIVE_PATH ${RELATIVE_NAME} PATH)

    if(RELATIVE_PATH)
        string(REPLACE "/" "\\" GROUP ${RELATIVE_PATH})
        if(${GENERATED})
            source_group("${GROUP}\\gen" FILES ${FILE})
        else()
            source_group(${GROUP} FILES ${FILE})
        endif()
    else()
        if(${GENERATED})
            source_group("gen" FILES ${FILE})
        else()
            source_group("" FILES ${FILE})
        endif()
    endif()
endfunction()

################################################################################
function(set_source_folders)
    cmake_parse_arguments(
        IGPA                # Save arguments with this prefix
        ""                  # Options with no arguments
        ""                  # Options with a single argument
        "FOLDERS;SOURCES"   # Options with multiple arguments
        ${ARGN}             # Arguments to parse
    )

    foreach(FILE ${IGPA_SOURCES})
        string(FIND ${FILE} ${CMAKE_CURRENT_BINARY_DIR} MANUAL) # if found, MANUAL will be 0
        if(NOT MANUAL)
            set(GENERATED TRUE)
        else()
            set(GENERATED FALSE)
        endif()
        foreach(FOLDER ${IGPA_FOLDERS})
            if(IS_ABSOLUTE ${FILE})
                file(RELATIVE_PATH RELATIVE_NAME ${FOLDER} ${FILE})
                if(NOT RELATIVE_NAME MATCHES "\\.\\..*")
                    set_source_folder(${GENERATED} ${FILE} ${FILE} ${FOLDER})
                endif()
            elseif(EXISTS ${FOLDER}/${FILE})
                set_source_folder(${GENERATED} ${FILE} ${FOLDER}/${FILE} ${FOLDER})
            endif()
        endforeach()
    endforeach()
endfunction()

################################################################################
function(set_default_target_folder TARGET)
    set(FILE ${CMAKE_CURRENT_LIST_FILE})
    file(RELATIVE_PATH RELATIVE_NAME ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_LIST_FILE})
    get_filename_component(RELATIVE_PATH ${RELATIVE_NAME} PATH)
    # Strip the final folder from the path (because this folder is used as the target name)
    string(REGEX REPLACE "[^/]+/?$" "" RELATIVE_PATH ${RELATIVE_PATH})
    if(RELATIVE_PATH)
        set_target_properties(${TARGET} PROPERTIES FOLDER ${RELATIVE_PATH})
    endif()
endfunction(set_default_target_folder)

################################################################################
# Set compile options
function(setup_compile_options target)
    # Set up compiler options
    if(MSVC)
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
            target_compile_options(${target}
                PRIVATE
                    /W4 # Warning level 4
                    /WX
                    /MP # Multi-processor compile
                    /sdl # Software Development Lifecycle security checks
                    /wd4204 # non-constant aggregate initalizers (supported in
                            # C99, which MSFT supports)
            )
        elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
            target_compile_options(${target}
                PRIVATE
                    -Wall               # Enable (almost) all warnings
                    -Wextra             # Enable additional warnings
                    -Werror             # Warnings as errors
                    -Wtype-limits       # Warn if a comparison is always true or always false due to the limited range of the data type
                    -Wno-missing-field-initializers # Disable warning when all struct members are not initialized
                    -Wunused-parameter
                    -Woverloaded-virtual
                    -Wpedantic          # Warn when using unstandard language extensions
                    -Wshadow            # Warn when variables are being shadowed
                    -Wpointer-bool-conversion   # Warn when implicitly using an address as a bool
                    -Wconversion        # Warn on any suspect implicit conversion
            )
        endif()

        if(${CMAKE_SYSTEM_NAME} STREQUAL WindowsStore)
            target_compile_options(${TARGET}
                PRIVATE
                    /ZW     # Allow MSVC extensions
                    /wd4530 # C++ exception handler used
            )
        endif()
        target_compile_definitions(${target}
            PRIVATE
                # Enable Unicode
                -D_UNICODE -DUNICODE
                # Do not define min and max macros
                -DNOMINMAX
                # Skip rarely-used Windows headers
                -DWIN32_LEAN_AND_MEAN
                # disable STL exceptions
                -D_HAS_EXCEPTIONS=0
                # Automatically call secure versions of unsecure methods (such as strncpy)
                -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1
                -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
        )
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU OR ${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        target_compile_options(${target}
            PRIVATE
                -Wall               # Enable (almost) all warnings
                -Wextra             # Enable additional warnings
                -Werror             # Warnings as errors
                -Wtype-limits       # Warn if a comparison is always true or always false due to the limited range of the data type
                -Wno-missing-field-initializers # Disable warning when all struct members are not initialized
                -Wunused-parameter
                -Woverloaded-virtual
                -Wpedantic          # Warn when using unstandard language extensions
                -Wshadow            # Warn when variables are being shadowed
                -Wconversion        # Warn on any suspect implicit conversion
        )

        # Add specific clang options
        if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
            target_compile_options(${target}
                PRIVATE
                    -Wpointer-bool-conversion   # Warn when implicitly using an address as a bool
                    -fobjc-arc #Enable Objective-C ARC
            )
        endif()
    endif()
endfunction()


################################################################################
function(add_bit_suffix_to_target_output TARGET)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "${TARGET}64")
    else()
        set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "${TARGET}32")
    endif()
endfunction()


################################################################################
function(igpa_add_executable IGPA_TARGET)
    cmake_parse_arguments(
        IGPA        # Save arguments with this prefix
        "WIN32;32_AND_64_BIT;ENABLE_RTTI_EXCEPTIONS"   # Options with no arguments
        "FOLDER"    # Options with a single argument
        "SOURCES"   # Options with multiple arguments
        ${ARGN}     # Arguments to parse
    )

    # Add the target using the given executable type.
    if(IGPA_WIN32)
        add_executable(${IGPA_TARGET} WIN32 ${IGPA_SOURCES} ${IGPA_UNPARSED_ARGUMENTS})
    else()
        add_executable(${IGPA_TARGET} ${IGPA_SOURCES} ${IGPA_UNPARSED_ARGUMENTS})
    endif()

    # Set project folder, defaults to match physical layout.
    if(IGPA_FOLDER)
        set_target_properties(${IGPA_TARGET} PROPERTIES FOLDER ${IGPA_FOLDER})
    else()
        set_default_target_folder(${IGPA_TARGET})
    endif()

    # By default, we don't build 32-bit executables. If a target needs both 32-
    # and 64-bit binaries, such as tests, rename the output accordingly. For
    # user-level applications, such as gpa-capture, default to 64-bit only,
    # disabling renaming and building of 32-bit binaries.
    if(IGPA_32_AND_64_BIT)
        add_bit_suffix_to_target_output(${IGPA_TARGET})
    else()
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set_target_properties(${IGPA_TARGET} PROPERTIES OUTPUT_NAME "${IGPA_TARGET}32")
            set_target_properties(${IGPA_TARGET} PROPERTIES
                EXCLUDE_FROM_ALL 1
                EXCLUDE_FROM_DEFAULT_BUILD 1)
        endif()
    endif()

    # Set default compile options.
    setup_compile_options(${IGPA_TARGET})

    if(IGPA_ENABLE_RTTI_EXCEPTIONS)
        message(STATUS "Exceptions on")
        if(MSVC)
            target_compile_options(${IGPA_TARGET}
                PRIVATE
                    "/GR"
                    "/EHsc"
            )
            target_compile_definitions(${IGPA_TARGET}
                PRIVATE
                    -D_HAS_EXCEPTIONS=1
            )
        else()
            target_compile_options(${IGPA_TARGET}
                PRIVATE
                    "-fno-rtti"
                    "-fno-exceptions"
            )
        endif()
    endif()

    get_property(TARGET_SOURCES TARGET ${IGPA_TARGET} PROPERTY SOURCES)

    # Organize project to match physical layout.
    set_source_folders(
        FOLDERS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
        SOURCES
            ${TARGET_SOURCES}
    )

    # Give access to the root binary dir for config.h
    target_include_directories(${IGPA_TARGET} PRIVATE ${CMAKE_BINARY_DIR})
endfunction()

################################################################################
function(igpa_add_library IGPA_TARGET)
    cmake_parse_arguments(
        IGPA                        # Save arguments with this prefix
        "NO_FORMAT;STATIC;SHARED"   # Options with no arguments
        "FOLDER"                    # Options with a single argument
        "SOURCES;GENERATED_SOURCES" # Options with multiple arguments
        ${ARGN}                     # Arguments to parse
    )

    # Add the target using the given library type.
    if(IGPA_STATIC)
        add_library(${IGPA_TARGET} STATIC ${IGPA_SOURCES} ${IGPA_GENERATED_SOURCES} ${IGPA_UNPARSED_ARGUMENTS})
    elseif(IGPA_SHARED)
        add_library(${IGPA_TARGET} SHARED ${IGPA_SOURCES} ${IGPA_GENERATED_SOURCES} ${IGPA_UNPARSED_ARGUMENTS})
    else()
        add_library(${IGPA_TARGET} ${IGPA_SOURCES} ${IGPA_GENERATED_SOURCES} ${IGPA_UNPARSED_ARGUMENTS})
    endif()

    # Name the output file according to bittage
    add_bit_suffix_to_target_output(${IGPA_TARGET})

    # Set project folder, defaults to match physical layout.
    if(IGPA_FOLDER)
        set_target_properties(${IGPA_TARGET} PROPERTIES FOLDER ${IGPA_FOLDER})
    else()
        set_default_target_folder(${IGPA_TARGET})
    endif()

    # Set default compile options.
    setup_compile_options(${IGPA_TARGET})

    get_property(TARGET_SOURCES TARGET ${IGPA_TARGET} PROPERTY SOURCES)

    # Organize project to match physical layout.
    set_source_folders(
        FOLDERS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
        SOURCES
            ${TARGET_SOURCES}
    )

    # Give access to the root binary dir for config.h
    target_include_directories(${IGPA_TARGET} PRIVATE ${CMAKE_BINARY_DIR})
endfunction()
