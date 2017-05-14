set(TARGET gfx)

set(SOURCES
    include/gfx/gfx.h
    gfx-internal.h
    gfx.c
)

if(HAVE_D3D12)
    list(APPEND SOURCES
        d3d12/gfx-d3d12.h
        d3d12/gfx-d3d12.cpp
    )
endif()
if(HAVE_METAL)
    list(APPEND SOURCES
        metal/gfx-metal.h
        metal/gfx-metal.m
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
            dxguid
            dxgi
            d3dx12
        PUBLIC
    )
endif()
if(HAVE_METAL)
    target_link_libraries(${TARGET}
        PRIVATE
            ${METAL_LIB}
            ${QUARTZCORE_LIB}
    )
    target_compile_options(${TARGET}
        PRIVATE
            -fno-objc-arc
    )
endif()

########
# test #
########
set(TEST_TARGET ${TARGET}-test)
set(TEST_SOURCES
    test/gfx-test.cpp
)

igpa_add_catchtest(${TEST_TARGET} ${TEST_SOURCES})

target_link_libraries(${TEST_TARGET}
    PRIVATE
        glfw
        ${TARGET}
)
