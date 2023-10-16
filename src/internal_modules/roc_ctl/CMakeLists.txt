file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_ctl STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_ctl PUBLIC
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)

target_link_libraries(roc_ctl roc_core roc_packet roc_netio)

set(TARGETS)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_ctl/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_ctl/*.cpp)
    target_sources(roc_ctl PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_ctl PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_ctl PRIVATE -DROC_MODULE=roc_ctl)
