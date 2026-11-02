cmake_minimum_required(VERSION 3.22)
project(Vitrine VERSION 0.3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Local test builds can point at an existing JUCE checkout: -DJUCE_DIR=/path/to/JUCE
if (DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} juce_build)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_gui_app(Vitrine
    COMPANY_NAME "Pouria Motabean"
    PRODUCT_NAME "Vitrine"
    ICON_BIG Resources/icon.png
    VERSION ${PROJECT_VERSION})

target_sources(Vitrine PRIVATE
    Source/Main.cpp
    Source/Library.cpp
    Source/Categories.cpp
    Source/Scanner.cpp
    Source/Theme.cpp
    Source/Thumbs.cpp
    Source/Sidebar.cpp
    Source/Grid.cpp
    Source/MainComponent.cpp
    Source/CubaseLink.cpp)

juce_add_binary_data(VitrineData SOURCES Resources/PouriaMotabean_Vitrine.js)

target_compile_definitions(Vitrine PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Vitrine,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Vitrine,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_USE_WINRT_MIDI=0)

target_link_libraries(Vitrine
    PRIVATE
        VitrineData
        juce::juce_gui_extra
        juce::juce_audio_processors
        juce::juce_audio_devices
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
