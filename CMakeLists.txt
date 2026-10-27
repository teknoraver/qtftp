cmake_minimum_required(VERSION 3.19)

project(qtftpgui LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(qtftpgui WIN32 MACOSX_BUNDLE
	main.cpp
	qtftpgui.cpp qtftpgui.h
	qtftp.cpp qtftp.h
	qtftpguiwidget.ui
	qtftpgui.qrc
)

if(WIN32)
	target_sources(qtftpgui PRIVATE qtftpgui.rc)
endif()

target_link_libraries(qtftpgui PRIVATE Qt6::Widgets Qt6::Network)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(qtftpgui PRIVATE -Wall -Wextra)
endif()

install(TARGETS qtftpgui
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
