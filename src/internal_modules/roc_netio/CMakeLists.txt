file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_netio STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_netio PUBLIC
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)
target_link_libraries(roc_netio roc_core roc_address roc_packet)

set(TARGETS target_posix target_libuv)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_netio/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_netio/*.cpp)
    target_sources(roc_netio PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_netio PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_netio PRIVATE -DROC_MODULE=roc_netio)
