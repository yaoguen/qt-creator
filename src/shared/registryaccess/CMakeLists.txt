if (WIN32)
    add_library(registryaccess STATIC registryaccess.cpp registryaccess.h)
    target_link_libraries(registryaccess PUBLIC advapi32 ole32 shell32 Qt5::Widgets)
    target_compile_definitions(registryaccess PRIVATE _UNICODE UNICODE)
    target_include_directories(registryaccess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    add_library(registryaccess INTERFACE)
endif()

if (WITH_SANITIZE)
  qtc_enable_sanitize(registryaccess ${SANITIZE_FLAGS})
endif()
