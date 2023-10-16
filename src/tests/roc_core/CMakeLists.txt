file(GLOB H_SOURCES CONFIGURE_DEPENDS test_*.h)
file(GLOB CPP_SOURCES CONFIGURE_DEPENDS test_*.cpp)

add_executable(roc-test-core)
target_sources(roc-test-core PRIVATE ${CPP_SOURCES} ${H_SOURCES} ../test_main.cpp)

#target_include_directories(roc-test-core PRIVATE
#        ${PROJECT_SOURCE_DIR}/src/internal_modules
#        ${PROJECT_SOURCE_DIR}/3rdparty/hedley
#)

target_link_libraries(roc-test-core roc_core CppUTest unwind)

#set(TARGETS target_posix)
#
#foreach (TARGET ${TARGETS})
#    file(GLOB TARGET_H_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_address/*.h)
#    file(GLOB TARGET_CPP_SOURCES CONFIGURE_DEPENDS ${TARGET}/roc_address/*.cpp)
#    target_sources(roc_address PRIVATE ${TARGET_H_SOURCES} ${TARGET_CPP_SOURCES})
#    target_include_directories(roc_address PUBLIC
#            ${TARGET}
#    )
#endforeach ()
#
target_compile_definitions(roc-test-core PRIVATE ROC_MODULE=roc-test-core)

target_compile_definitions(roc-test-core PRIVATE CPPUTEST_USE_MEM_LEAK_DETECTION=0)