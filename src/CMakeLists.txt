set(TARGET asteroids)

set(SOURCES
    main.c
)

igpa_add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    PUBLIC
)

target_include_directories(${TARGET}
    PRIVATE
    PUBLIC
)
