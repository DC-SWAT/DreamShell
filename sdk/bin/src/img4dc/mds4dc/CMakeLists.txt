
set(MDS4DC_SOURCES
    src/console.c
    src/imgwrite.c
    src/main.c
    src/mdfwrt.c
    src/mdsaudio.c
    src/mdsdata.c
    src/tools.c
)

add_executable(mds4dc ${MDS4DC_SOURCES})
target_include_directories(mds4dc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/edc/inc
)

target_link_libraries(mds4dc edc)
