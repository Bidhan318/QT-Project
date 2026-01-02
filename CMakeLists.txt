cmake_minimum_required(VERSION 3.16)
project(QT_project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages directly (no need for Qt5 fallback with 6.10.1)
find_package(Qt6 6.10.1 REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        main.cpp
        login_window.cpp
        login_window.h
        login_window.ui
        mainwindow.h
        mainwindow.cpp
        mainwindow.ui
        serverwindow.h
        serverwindow.cpp
        serverwindow.ui
        port.h
)

qt_add_executable(QT_project
    ${PROJECT_SOURCES}
    resources.qrc
)

# Link Qt6 libraries
target_link_libraries(QT_project PRIVATE
    Qt6::Widgets
    Qt6::Network
)

set_target_properties(QT_project PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.QT_project
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QT_project
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(QT_project)
