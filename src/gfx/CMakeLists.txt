set(TARGET gfx)

set(SOURCES
    include/gfx/gfx.h
    gfx.c
)

igpa_add_library(${TARGET} STATIC ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    PUBLIC
)

target_include_directories(${TARGET}
    PRIVATE
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
