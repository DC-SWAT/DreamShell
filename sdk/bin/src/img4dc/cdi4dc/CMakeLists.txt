
set(CDI4DC_SOURCES
    src/cdiaudio.c
    src/cdidata.c
    src/common.c
    src/main.c
    src/tools.c
)

add_executable(cdi4dc ${CDI4DC_SOURCES})
target_include_directories(cdi4dc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/edc/inc
    ${CMAKE_SOURCE_DIR}/common/inc
)

target_link_libraries(cdi4dc img4dc-common edc)
