cmake_minimum_required(VERSION 3.21)

project(chat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Profiling helpers (perf/flamegraphs): keep symbols + reliable stacks.
# Use with: -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAPPCHAT_PROFILING=ON
option(APPCHAT_PROFILING "Enable profiling-friendly compiler/linker flags" OFF)

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick Svg)

qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0002 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(appchat
    MANUAL_FINALIZATION
    main.cpp

    src/net/ProtocolClient.cpp
    src/controller/ClientController.cpp
    src/service/MessageService.cpp
    src/model/ContactListModel.cpp
    src/model/ContactProxyModel.cpp
    src/model/ChatHistoryModel.cpp
)

set_property(TARGET appchat PROPERTY
    QT_ANDROID_PACKAGE_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/android
)

target_include_directories(appchat PRIVATE src)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appchat
    URI chat
    VERSION 1.0

    QML_FILES
        Main.qml
        qml/LoginFrame.qml
        qml/RegisterFrame.qml
        qml/ChatShell.qml
        qml/MobileChatShell.qml
        qml/HeaderBar.qml
        qml/ContactsFrame.qml
        qml/ChatView.qml
        qml/InputBar.qml
        qml/Theme.qml
        qml/Toast.qml
        qml/ToastHost.qml
        qml/SettingsDrawer.qml
        qml/ConnectionSettings.qml
        qml/MobileLoginFrame.qml
        qml/MobileRegisterFrame.qml
        Utils/CustomInput.qml
        Utils/CustomCheckBox.qml
        Utils/CustomButton.qml
        Utils/CustomLabel.qml

    RESOURCES
        images/logo.png
        images/back-icon.png
        images/message-icon.png
        images/lock.png
        images/settings-icon.png
        images/patterns/hearts.svg
        images/patterns/diamonds.svg
)

target_link_libraries(appchat
    PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Svg
)

if(APPCHAT_PROFILING)
    # Needed for Qt Creator QML Profiler (starts the QML debug/profiler server).
    if(NOT ANDROID)
        target_compile_definitions(appchat PRIVATE QT_QML_DEBUG)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(appchat PRIVATE -g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
        target_link_options(appchat PRIVATE -g -Wl,--build-id)
    endif()
endif()

if(NOT ANDROID)
    set_target_properties(appchat PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING
            ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
endif()

include(GNUInstallDirs)
install(TARGETS appchat
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_finalize_executable(appchat)
