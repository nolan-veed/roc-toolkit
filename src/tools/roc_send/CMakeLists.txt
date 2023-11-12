file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

file(GLOB GGO_H_SOURCES CONFIGURE_DEPENDS build/src/x86_64-pc-linux-gnu/clang-14.0.0-debug/tools/roc_send/*.h)
file(GLOB GGO_CPP_SOURCES CONFIGURE_DEPENDS build/src/x86_64-pc-linux-gnu/clang-14.0.0-debug/tools/roc_send/*.cpp)

add_executable(roc_send ${CPP_SOURCES} ${H_SOURCES} ${GGO_H_SOURCES} ${GGO_CPP_SOURCES})

target_include_directories(roc_send PUBLIC
#        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
        ${PROJECT_SOURCE_DIR}/build/src/x86_64-pc-linux-gnu/clang-14.0.0-debug/tools/
)

target_link_libraries(roc_send roc_core roc_address roc_netio roc_node roc_pipeline roc_sndio)

#set(TARGETS target_c11 target_gnu target_libunwind target_pc target_posix target_posix_ext target_posix_pc)
#
#foreach (TARGET ${TARGETS})
#    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_sndio/*.h)
#    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_sndio/*.cpp)
#    target_sources(roc_send PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
#    target_include_directories(roc_send PUBLIC
#            ${TARGET}
#    )
#endforeach ()

target_compile_definitions(roc_send PRIVATE -DROC_MODULE=roc_send)
