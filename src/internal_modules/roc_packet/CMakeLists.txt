file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_packet STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_packet PRIVATE
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)

target_link_libraries(roc_packet roc_core roc_address)

set(TARGETS target_libuv)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_packet/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_packet/*.cpp)
    target_sources(roc_packet PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_packet PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_packet PRIVATE -DROC_MODULE=roc_packet)
