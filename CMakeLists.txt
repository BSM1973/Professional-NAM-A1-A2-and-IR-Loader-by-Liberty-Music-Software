cmake_minimum_required(VERSION 3.24)
project(LibertyNeuralMatrix VERSION 0.8.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep UTF-8 French labels and error messages intact in MSVC.
if(MSVC)
  add_compile_options(/utf-8)
endif()

include(FetchContent)
# Reproducible tags are intentionally kept fixed to avoid unexpected API breakage.
# Builds are started MANUALLY to conserve GitHub Actions usage.

# Pin dependencies for reproducible public GitHub builds.
FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

# NAM Core A2 capable, MIT licensed, with Eigen/nlohmann submodules.
FetchContent_Declare(namcore GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
  GIT_TAG v0.5.4 GIT_SHALLOW TRUE GIT_SUBMODULES_RECURSE TRUE)
FetchContent_GetProperties(namcore)
if(NOT namcore_POPULATED)
  FetchContent_Populate(namcore)
endif()
file(GLOB NAM_SRC CONFIGURE_DEPENDS "${namcore_SOURCE_DIR}/NAM/*.cpp" "${namcore_SOURCE_DIR}/NAM/*.c" "${namcore_SOURCE_DIR}/NAM/*/*.cpp")
add_library(liberty_nam_core STATIC ${NAM_SRC})
target_include_directories(liberty_nam_core SYSTEM PUBLIC
  "${namcore_SOURCE_DIR}/NAM"
  "${namcore_SOURCE_DIR}/Dependencies/eigen"
  "${namcore_SOURCE_DIR}/Dependencies/nlohmann"
  "${namcore_SOURCE_DIR}/Dependencies/AudioDSPTools/dsp")
target_compile_features(liberty_nam_core PUBLIC cxx_std_20)
target_compile_definitions(liberty_nam_core PUBLIC NAM_SAMPLE_FLOAT NAM_ENABLE_A2_FAST)
set_target_properties(liberty_nam_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(APPLE)
  set(LIBERTY_PLUGIN_FORMATS AU VST3 Standalone)
else()
  set(LIBERTY_PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(LibertyNeuralMatrix
  COMPANY_NAME "Liberty Music Software"
  BUNDLE_ID "com.libertymusicsoftware.neuralmatrix"
  PLUGIN_MANUFACTURER_CODE LMSW
  PLUGIN_CODE LNRM
  FORMATS ${LIBERTY_PLUGIN_FORMATS}
  PRODUCT_NAME "Liberty Neural Matrix"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(LibertyNeuralMatrix)

target_sources(LibertyNeuralMatrix PRIVATE
  Source/PluginProcessor.h Source/PluginProcessor.cpp
  Source/PluginEditor.h Source/PluginEditor.cpp
  Source/AmpVisualDetector.h Source/AmpArtwork.h
  Source/LibraryBrowser.h Source/CabArtwork.h Source/SessionStateCodec.h)
target_compile_definitions(LibertyNeuralMatrix PRIVATE
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
# NAM registers WaveNet/LSTM/ConvNet/Linear using static TU constructors.
# Without whole-archive, the plugin host linker strips those translation units,
# resulting in "No config parser registered for architecture: WaveNet".
# CMake 3.24 supports WHOLE_ARCHIVE for Apple ld64 and MSVC link.exe.
target_link_libraries(LibertyNeuralMatrix PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,liberty_nam_core>"
  juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

if(WIN32)
  set_target_properties(LibertyNeuralMatrix PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Smoke test: catches missing NAM architecture registrars before publishing artifacts.
enable_testing()
add_executable(liberty_nam_registry_smoke tests/nam_registry_smoke.cpp)
target_link_libraries(liberty_nam_registry_smoke PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,liberty_nam_core>")
target_compile_features(liberty_nam_registry_smoke PRIVATE cxx_std_20)
add_test(NAME NAM_Registries COMMAND liberty_nam_registry_smoke)

# Pure C++ detector tests: no neural model files, external downloads or JUCE test
# helpers needed. Keeps false visual identifications under control.
add_executable(liberty_amp_visual_detect tests/amp_visual_detect_test.cpp)
target_include_directories(liberty_amp_visual_detect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(liberty_amp_visual_detect PRIVATE cxx_std_20)
add_test(NAME Amp_Visual_Detection COMMAND liberty_amp_visual_detect)

# JUCE session codec roundtrip: exercises the exact save/restore schema used by
# the plugin without needing to host a DAW in CI.
juce_add_console_app(liberty_session_state_smoke PRODUCT_NAME "LNM Session Test")
juce_generate_juce_header(liberty_session_state_smoke)
target_sources(liberty_session_state_smoke PRIVATE tests/session_state_smoke.cpp)
target_include_directories(liberty_session_state_smoke PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_link_libraries(liberty_session_state_smoke PRIVATE juce::juce_data_structures)
target_compile_features(liberty_session_state_smoke PRIVATE cxx_std_20)
add_test(NAME Session_State_Roundtrip COMMAND liberty_session_state_smoke)

# Real NAM files from the pinned upstream NAM Core release. They are downloaded
# once by CMake FetchContent and never committed as extra third-party binaries.
add_executable(liberty_nam_fixture_smoke tests/nam_fixture_smoke.cpp)
target_link_libraries(liberty_nam_fixture_smoke PRIVATE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,liberty_nam_core>")
target_compile_features(liberty_nam_fixture_smoke PRIVATE cxx_std_20)
foreach(fixture wavenet lstm A2)
  add_test(NAME NAM_Real_${fixture}
    COMMAND liberty_nam_fixture_smoke "${namcore_SOURCE_DIR}/example_models/${fixture}.nam")
endforeach()
