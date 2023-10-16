file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_sdp STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_sdp PUBLIC
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)

target_link_libraries(roc_sdp roc_core roc_address)

set(TARGETS target_c11 target_gnu target_libunwind target_pc target_posix target_posix_ext target_posix_pc)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_sdp/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_sdp/*.cpp)
    target_sources(roc_sdp PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_sdp PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_sdp PRIVATE -DROC_MODULE=roc_sdp)
