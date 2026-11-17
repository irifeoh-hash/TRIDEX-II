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

juce_add_plugin(Tridex80
    VST3_AUTO_MANIFEST ${TRIDEX_VST3_MANIFEST}
    COMPANY_NAME "Tridex Audio"
    PLUGIN_MANUFACTURER_CODE Tdxa
    PLUGIN_CODE Tx80
    FORMATS VST3 Standalone
    PRODUCT_NAME "Tridex 80"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Tridex80 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Tridex80 PRIVATE Source)

target_compile_definitions(Tridex80 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0)

target_link_libraries(Tridex80
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# MinGW cross builds: MSVC's #pragma comment(lib) is ignored, so name the Windows libraries explicitly.
if(MINGW)
    target_link_libraries(Tridex80 PRIVATE
        winmm shlwapi version ws2_32 wininet ole32 oleaut32 uuid imm32 comdlg32 gdi32 user32 shell32
        dwrite d2d1 d3d11 dxgi dxguid uxtheme comctl32 setupapi crypt32 rpcrt4 advapi32 kernel32 dbghelp
        dcomp dwmapi sapi ksuser ksguid)
    target_sources(Tridex80 PRIVATE Source/win/MinGWCompat.cpp)
endif()

# Headless DSP render test (no JUCE dependency)
add_executable(render_test tests/render_test.cpp)
target_include_directories(render_test PRIVATE Source)
