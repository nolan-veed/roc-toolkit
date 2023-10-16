file(GLOB H_SOURCES CONFIGURE_DEPENDS *.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(roc_address STATIC ${CPP_SOURCES} ${H_SOURCES})

target_include_directories(roc_address PRIVATE
        ${PROJECT_SOURCE_DIR}/src/internal_modules
        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
)

target_link_libraries(roc_address roc_core)

set(TARGETS target_posix)

foreach (TARGET ${TARGETS})
    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_address/*.h)
    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_address/*.cpp)
    target_sources(roc_address PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
    target_include_directories(roc_address PUBLIC
            ${TARGET}
    )
endforeach ()

target_compile_definitions(roc_address PRIVATE -DROC_MODULE=roc_address)
