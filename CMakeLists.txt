cmake_minimum_required(VERSION 3.22)

project(OmniportMIDI VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch JUCE from GitHub (tag 8.0.8)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# GUI Application target
# ---------------------------------------------------------------------------
juce_add_gui_app(OmniportMIDI
    PRODUCT_NAME        "OmniportMIDI"
    VERSION             "0.1.0"
    BUNDLE_ID           "com.omniport.OmniportMIDI"
    COMPANY_NAME        "Omniport"
    COMPANY_WEBSITE     "https://omniport.app"
    MICROPHONE_PERMISSION_ENABLED  FALSE
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
target_sources(OmniportMIDI
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
)

# Generate JuceHeader.h (optional but conventional)
juce_generate_juce_header(OmniportMIDI)

# ---------------------------------------------------------------------------
# Compile definitions (JUCE conventions)
# ---------------------------------------------------------------------------
target_compile_definitions(OmniportMIDI
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OmniportMIDI,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OmniportMIDI,JUCE_VERSION>"
)

# ---------------------------------------------------------------------------
# Link required JUCE modules
# ---------------------------------------------------------------------------
target_link_libraries(OmniportMIDI
    PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_audio_devices
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
