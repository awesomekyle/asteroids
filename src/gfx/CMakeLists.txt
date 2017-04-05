set(TARGET gfx)

set(SOURCES
    include/gfx/gfx.h
    gfx.c
)

if(HAVE_D3D12)
    list(APPEND SOURCES
        d3d12/gfx-d3d12.h
        d3d12/gfx-d3d12.cpp
    )
endif()

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

if(HAVE_D3D12)
    target_link_libraries(${TARGET}
        PRIVATE
            d3d12
        PUBLIC
    )
endif()
