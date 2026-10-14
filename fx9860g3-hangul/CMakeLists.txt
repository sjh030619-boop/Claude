# fxSDK project for fx-9860G (fx-9860G III runs .g1a add-ins).
# Build:  fxsdk build-fx     ->  dist/Hangul.g1a
cmake_minimum_required(VERSION 3.15)
project(Hangul)

include(GenerateG1A)
include(Fxconv)
find_package(Gint 2.1 REQUIRED)

set(SOURCES
  src/main.c
  src/hangul.c
  src/fakecalc.c
  src/font_data.c
  src/notes.c
)

add_executable(hangul ${SOURCES})
target_compile_options(hangul PRIVATE -Wall -Wextra -Os)
target_link_libraries(hangul Gint::Gint)

generate_g1a(TARGET hangul OUTPUT "Hangul.g1a"
  NAME "Hangul" ICON assets-fx/icon.png)
