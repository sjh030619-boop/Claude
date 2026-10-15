# fxSDK project for fx-9860G (fx-9860G III runs .g1a add-ins).
# Build:  fxsdk build-fx     ->  dist/Hangul.g1a
cmake_minimum_required(VERSION 3.15)
project(Hangul)

# Add-in name / output file come from the note set: python3 tools/build_assets.py --set <id>
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/addin.cmake")
  message(FATAL_ERROR "src/addin.cmake missing: run  python3 tools/build_assets.py --set <id>  first")
endif()
include("${CMAKE_SOURCE_DIR}/src/addin.cmake")

include(GenerateG1A)
include(Fxconv)
find_package(Gint 2.1 REQUIRED)

set(SOURCES
  src/main.c
  src/hangul.c
  src/fakecalc.c
  src/fakecalc_data.c
  src/font_data.c
  src/notes.c
)

add_executable(hangul ${SOURCES})
target_compile_options(hangul PRIVATE -Wall -Wextra -Os)
target_link_libraries(hangul Gint::Gint)

generate_g1a(TARGET hangul OUTPUT "${ADDIN_OUTPUT}"
  NAME "${ADDIN_NAME}" ICON assets-fx/icon.png)
