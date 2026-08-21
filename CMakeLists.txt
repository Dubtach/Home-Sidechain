cmake_minimum_required(VERSION 3.25)

project(HomeSidechain VERSION 17.4.0 LANGUAGES C CXX)

set(COMPANY_NAME "Dubtach")
set(FORMATS Standalone AU VST3 AUv3)

# Keep JUCE as the normal Pamplejuce git submodule in the repository root.
add_subdirectory(JUCE)

# -----------------------------------------------------------------------------
# Home-Sidechain Trigger
# -----------------------------------------------------------------------------
juce_add_plugin(HomeSidechainTrigger
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "com.dubtach.homesidechain.trigger"
    PLUGIN_MANUFACTURER_CODE Dubt
    PLUGIN_CODE Hstg
    VST3_CATEGORIES Generator
    NEEDS_MIDI_OUTPUT TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    FORMATS ${FORMATS}
    PRODUCT_NAME "Home-Sidechain Trigger"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(HomeSidechainTrigger)

target_sources(HomeSidechainTrigger PRIVATE
    source/Shared/SidechainCommon.h
    source/Shared/HomeLink.h
    source/Trigger/PluginProcessor.h
    source/Trigger/PluginProcessor.cpp
    source/Trigger/PluginEditor.h
    source/Trigger/PluginEditor.cpp
)

target_link_libraries(HomeSidechainTrigger PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
)

target_compile_definitions(HomeSidechainTrigger PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# -----------------------------------------------------------------------------
# Home-Sidechain Receiver
# -----------------------------------------------------------------------------
juce_add_plugin(HomeSidechainReceiver
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "com.dubtach.homesidechain.receiver"
    PLUGIN_MANUFACTURER_CODE Dubt
    PLUGIN_CODE Hscr
    VST3_CATEGORIES Dynamics
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    FORMATS ${FORMATS}
    PRODUCT_NAME "Home-Sidechain Receiver"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(HomeSidechainReceiver)

target_sources(HomeSidechainReceiver PRIVATE
    source/Shared/SidechainCommon.h
    source/Shared/HomeLink.h
    source/Receiver/PluginProcessor.h
    source/Receiver/PluginProcessor.cpp
    source/Receiver/PluginEditor.h
    source/Receiver/PluginEditor.cpp
)

target_link_libraries(HomeSidechainReceiver PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
)

target_compile_definitions(HomeSidechainReceiver PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)
