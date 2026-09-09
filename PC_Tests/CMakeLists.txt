cmake_minimum_required(VERSION 3.10)
project(GiacLovyanGFX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------- OS detection -------------------
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# ------------------- Find GMP/MPFR: prefer prebuilt static, fallback to system -------------------
function(find_static_library LIB_NAME OUT VENDOR_PATH)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    find_library(
        FOUND_${LIB_NAME}_STATIC
        ${LIB_NAME}
        NO_DEFAULT_PATH
        PATHS ${VENDOR_PATH}
        )
    if (FOUND_${LIB_NAME}_STATIC)
        get_filename_component(ABS_FILE ${FOUND_${LIB_NAME}_STATIC} ABSOLUTE)
        set(${OUT} ${ABS_FILE} PARENT_SCOPE)
    else()
        set(${OUT} "" PARENT_SCOPE)
    endif()
endfunction()

set(PREBUILT_DIR ${CMAKE_SOURCE_DIR}/src/jni/prebuilt)

if(LINUX)
    message(STATUS "Looking for prebuilt gmp/mpfr (Linux)")
    find_static_library(gmp GMP_STATIC ${PREBUILT_DIR}/linux/x86-64/)
    find_static_library(mpfr MPFR_STATIC ${PREBUILT_DIR}/linux/x86-64/)
endif()

if(APPLE)
    message(STATUS "Looking for prebuilt gmp/mpfr (Mac)")
    find_static_library(gmp GMP_STATIC ${PREBUILT_DIR}/maccatalyst/x86_64/)
    find_static_library(mpfr MPFR_STATIC ${PREBUILT_DIR}/maccatalyst/x86_64/)
endif()

if(WIN32)
    if("$ENV{MSYSTEM}" STREQUAL "CLANG64")
        message(STATUS "Looking for prebuilt gmp/mpfr for clang64")
        find_static_library(gmp GMP_STATIC ${PREBUILT_DIR}/windows/clang64/)
        find_static_library(mpfr MPFR_STATIC ${PREBUILT_DIR}/windows/clang64/)
    endif()
    if("$ENV{MSYSTEM}" STREQUAL "CLANG32")
        message(STATUS "Looking for prebuilt gmp/mpfr for clang32")
        find_static_library(gmp GMP_STATIC ${PREBUILT_DIR}/windows/clang32/)
        find_static_library(mpfr MPFR_STATIC ${PREBUILT_DIR}/windows/clang32/)
    endif()
endif()

# Fallback to system libraries if static ones are missing
if(NOT GMP_STATIC OR NOT MPFR_STATIC)
    message(STATUS "Prebuilt static libraries not found; searching system GMP/MPFR")
    find_library(GMP_SYSTEM gmp)
    find_library(MPFR_SYSTEM mpfr)
    if(NOT GMP_SYSTEM OR NOT MPFR_SYSTEM)
        message(FATAL_ERROR "Could not find GMP or MPFR. Please install libgmp-dev and libmpfr-dev (or equivalent).")
    endif()
    set(GMP_LIB ${GMP_SYSTEM})
    set(MPFR_LIB ${MPFR_SYSTEM})
else()
    set(GMP_LIB ${GMP_STATIC})
    set(MPFR_LIB ${MPFR_STATIC})
endif()

# ------------------- Compiler settings -------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

# ------------------- Giac sources (excluding its own main) -------------------
set(GIAC_SOURCES
    giac/cpp/alg_ext.cc
    giac/cpp/cocoa.cc
    giac/cpp/csturm.cc
    giac/cpp/derive.cc
    giac/cpp/desolve.cc
    giac/cpp/ezgcd.cc
    giac/cpp/freeglut_stroke_roman.c
    giac/cpp/gauss.cc
    giac/cpp/gausspol.cc
    giac/cpp/gen.cc
    giac/cpp/global.cc
    giac/cpp/help.cc
    giac/cpp/identificateur.cc
    giac/cpp/ifactor.cc
    giac/cpp/index.cc
    giac/cpp/input_lexer.cc
    giac/cpp/input_parser.cc
    giac/cpp/intg.cc
    giac/cpp/intgab.cc
    giac/cpp/isom.cc
    giac/cpp/lin.cc
    giac/cpp/maple.cc
    giac/cpp/mathml.cc
    giac/cpp/misc.cc
    giac/cpp/modfactor.cc
    giac/cpp/modpoly.cc
    giac/cpp/moyal.cc
    giac/cpp/opengl.cc
    giac/cpp/pari.cc
    giac/cpp/permu.cc
    giac/cpp/plot.cc
    giac/cpp/plot3d.cc
    giac/cpp/prog.cc
    giac/cpp/quater.cc
    giac/cpp/risch.cc
    giac/cpp/rpn.cc
    giac/cpp/series.cc
    giac/cpp/solve.cc
    giac/cpp/sparse.cc
    giac/cpp/subst.cc
    giac/cpp/sym2poly.cc
    giac/cpp/symbolic.cc
    giac/cpp/tex.cc
    giac/cpp/threaded.cc
    giac/cpp/ti89.cc
    giac/cpp/tinymt32.cc
    giac/cpp/TmpFGLM.cpp
    giac/cpp/TmpLESystemSolver.cpp
    giac/cpp/unary.cc
    giac/cpp/usual.cc
    giac/cpp/vecteur.cc
)

# ------------------- LovyanGFX + SDL sources -------------------
file(GLOB LGFX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
    src/*.cpp                         # Your application code (main included)
    src/fonts/*.cpp
    src/fonts/*.c
    LovyanGFX/src/lgfx/Fonts/efont/*.c
    LovyanGFX/src/lgfx/Fonts/IPA/*.c
    LovyanGFX/src/lgfx/Fonts/lvgl/*.c
    LovyanGFX/src/lgfx/utility/*.c
    LovyanGFX/src/lgfx/v1/*.cpp
    LovyanGFX/src/lgfx/v1/lv_font/*.c
    LovyanGFX/src/lgfx/v1/misc/*.cpp
    LovyanGFX/src/lgfx/v1/panel/Panel_Device.cpp
    LovyanGFX/src/lgfx/v1/panel/Panel_FrameBufferBase.cpp
    LovyanGFX/src/lgfx/v1/platforms/sdl/*.cpp
)

# ------------------- Combined executable -------------------
add_executable(giac_lgfxsdl
    ${GIAC_SOURCES}
    ${LGFX_SOURCES}
)

# ------------------- Include directories -------------------
target_include_directories(giac_lgfxsdl PRIVATE
    giac/headers
    LovyanGFX/src/
    include/   # <- Remove this line if you don't have a custom 'include' folder
    include/fonts/
)

# ------------------- Compile definitions -------------------
target_compile_definitions(giac_lgfxsdl PRIVATE
    # From Giac
    -DHAVE_NO_HOME_DIRECTORY
    -DHAVE_ALLOCA_H
    -DGIAC_GGB
    -DIN_GIAC
    -DHAVE_LIB_PTHREAD
    -DGIAC_GENERIC_CONSTANTS
    -DTIMEOUT
    -DSIZEOF_VOID_P=8
    -DHAVE_LIBMPFR
    -DVERSION="1.2.3"
    # From LovyanGFX
    -DLGFX_SDL
)

# Platform-specific definitions
if(UNIX)
    target_compile_definitions(giac_lgfxsdl PRIVATE
        -DHAVE_SYS_TIMES_H
        -DHAVE_UNISTD_H
        -DHAVE_SYS_TIME_H
        -DHAVE_SYSCONF
    )
endif()

if(WIN32)
    target_compile_definitions(giac_lgfxsdl PRIVATE
        -DGIAC_MPQS
        -D__MINGW_H
        -DMINGW32
        -DHAVE_NO_SYS_TIMES_H
        -DHAVE_NO_SYS_RESOURCE_WAIT_H
        -DHAVE_NO_PWD_H
        -DHAVE_NO_CWD
        -DNO_CLOCK
        -Dusleep=
        -DYY_NO_UNISTD_H
    )
endif()

if(APPLE)
    target_compile_definitions(giac_lgfxsdl PRIVATE -DDONT_USE_LIBLAPLACK)
endif()

# ------------------- Link libraries -------------------
# Order matters: MPFR must precede GMP
target_link_libraries(giac_lgfxsdl
    ${MPFR_LIB}
    ${GMP_LIB}
)

# SDL2 and system libraries
if(LINUX)
    find_package(SDL2 REQUIRED)
    target_include_directories(giac_lgfxsdl PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(giac_lgfxsdl ${SDL2_LIBRARIES} -lpthread)
    target_link_options(giac_lgfxsdl PRIVATE -static-libgcc -static-libstdc++)
elseif(WIN32)
    # Adjust SDL2 path if needed; here we assume C:/SDL2
    target_include_directories(giac_lgfxsdl PRIVATE "C:/SDL2/include")
    target_link_libraries(giac_lgfxsdl "C:/SDL2/lib/x64/SDL2.lib")
    target_link_options(giac_lgfxsdl PRIVATE -static-libgcc -static-libstdc++)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
elseif(APPLE)
    find_package(SDL2 REQUIRED COMPONENTS SDL2)
    target_link_libraries(giac_lgfxsdl PRIVATE SDL2::SDL2)
    # If you need Homebrew's libs for GMP/MPFR, uncomment:
    # target_link_directories(giac_lgfxsdl PRIVATE /usr/local/lib)
endif()

# Optional: strip symbols
set_target_properties(giac_lgfxsdl PROPERTIES LINK_FLAGS -s)
