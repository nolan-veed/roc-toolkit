file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_audio STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_audio PRIVATE
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)

target_link_libraries(roc_audio roc_core roc_packet)

set(TARGETS target_posix)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_audio/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_audio/*.cpp)
    target_sources(roc_audio PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_audio PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_audio PRIVATE -DROC_MODULE=roc_audio)
