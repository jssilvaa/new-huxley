cmake_minimum_required(VERSION 3.16)

project(chat VERSION 0.1 LANGUAGES CXX)

# For clangd/IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

# Executable cpp sources 
qt_add_executable(appchat
    main.cpp

    src/net/ProtocolClient.cpp
    src/controller/ClientController.cpp
    src/service/MessageService.cpp
    src/model/ContactListModel.cpp
    src/model/ContactProxyModel.cpp
    src/model/ChatHistoryModel.cpp
)

target_include_directories(appchat 
    PRIVATE src
)

# mark QML singletons
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# QML modules and resources
qt_add_qml_module(appchat
    URI chat
    VERSION 1.0

    QML_FILES
        Main.qml
        qml/LoginFrame.qml
        qml/RegisterFrame.qml
        qml/ChatShell.qml
        qml/HeaderBar.qml
        qml/ContactsFrame.qml
        qml/ChatView.qml
        qml/InputBar.qml
        qml/Theme.qml
        qml/Toast.qml
        qml/ToastHost.qml
        qml/SettingsDrawer.qml
        Utils/CustomInput.qml
        Utils/CustomCheckBox.qml
        Utils/CustomButton.qml
        
    RESOURCES
        images/logo.png
        images/message-icon.png
        images/lock.png
)

# Platform specific properties
set_target_properties(appchat PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appchat
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING 
        ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# Link QT libraries
target_link_libraries(appchat
    PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS appchat
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
