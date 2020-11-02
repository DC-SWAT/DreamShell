
set(COMMON_SRC 
    src/console.c
)

add_library(img4dc-common ${COMMON_SRC})
target_include_directories(img4dc-common PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
