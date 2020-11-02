
set(EDC_SOURCES
    src/edc_ecc.c
    src/libedc.c
    src/patch.c
)

add_library(edc ${EDC_SOURCES})
target_include_directories(edc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
