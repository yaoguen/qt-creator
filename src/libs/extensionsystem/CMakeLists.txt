add_qtc_library(ExtensionSystem
  DEPENDS Aggregation Utils
  PUBLIC_DEPENDS Qt5::Core Qt5::Widgets
  SOURCES
    extensionsystem_global.h
    extensionsystemtr.h
    invoker.cpp invoker.h
    iplugin.cpp iplugin.h iplugin_p.h
    optionsparser.cpp optionsparser.h
    plugindetailsview.cpp plugindetailsview.h
    pluginerroroverview.cpp pluginerroroverview.h
    pluginerrorview.cpp pluginerrorview.h
    pluginmanager.cpp pluginmanager.h pluginmanager_p.h
    pluginspec.cpp pluginspec.h pluginspec_p.h
    pluginview.cpp pluginview.h
  EXPLICIT_MOC
    pluginmanager.h
    pluginmanager_p.h
  SKIP_AUTOMOC pluginmanager.cpp
)

find_package(Qt5 COMPONENTS Test QUIET)

extend_qtc_library(ExtensionSystem
  CONDITION TARGET Qt5::Test
  DEPENDS Qt5::Test
  DEFINES WITH_TESTS
)
