cmake_minimum_required(VERSION 3.22)
project(Tridex80 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout if present (-DJUCE_PATH=...), otherwise fetch it.
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE")
if(EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# The VST3 manifest helper must run the built plugin, which isn't possible when cross-compiling.
if(CMAKE_CROSSCOMPILING)
    set(TRIDEX_VST3_MANIFEST FALSE)
else()
    set(TRIDEX_VST3_MANIFEST TRUE)
endif()

# Plugin formats. The standalone app is only for local UI testing (off by default).
option(TRIDEX_STANDALONE "Also build the standalone app (local testing only)" OFF)
set(TRIDEX_FORMATS VST3)
if(APPLE)
    list(APPEND TRIDEX_FORMATS AU)   # Audio Unit for Logic Pro / GarageBand
endif()
if(TRIDEX_STANDALONE)
    list(APPEND TRIDEX_FORMATS Standalone)
endif()

juce_add_plugin(Tridex80
    VST3_AUTO_MANIFEST ${TRIDEX_VST3_MANIFEST}
    COMPANY_NAME "Tridex Audio"
    PLUGIN_MANUFACTURER_CODE Tdxa
    PLUGIN_CODE Tx82
    FORMATS ${TRIDEX_FORMATS}
    PRODUCT_NAME "Tridex II"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Embedded artwork (I Rife logo)
juce_add_binary_data(TridexAssets SOURCES Resources/irife_logo.png Resources/tridex_logo.png)
set_target_properties(TridexAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(Tridex80 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Tridex80 PRIVATE Source)

target_compile_definitions(Tridex80 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    # Plugin-only build: leave out JUCE features Tridex 80 never uses
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_CDREADER=0
    JUCE_USE_CDBURNER=0
    JUCE_PLUGINHOST_VST3=0
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0)

target_link_libraries(Tridex80
    PRIVATE juce::juce_audio_utils TridexAssets
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Headless DSP render test (no JUCE dependency)
add_executable(render_test tests/render_test.cpp)
target_include_directories(render_test PRIVATE Source)
