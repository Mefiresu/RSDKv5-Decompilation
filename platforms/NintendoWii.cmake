find_package(PkgConfig REQUIRED)

add_executable(RetroEngine ${RETRO_FILES})

set(RETRO_SUBSYSTEM "GX" CACHE STRING "The subsystem to use")
# Two sets of subsystems are available on the Wii:
# - GX/AESND/wiiuse: common low level APIs provided by libogc, faster and better looking (recommended)
# - SDL2: experimental support, slower but usable
if(RETRO_SUBSYSTEM STREQUAL "GX")
    target_link_libraries(RetroEngine aesnd)
elseif(RETRO_SUBSYSTEM STREQUAL "SDL2")
    message(WARNING "SDL2 support is experimental!")
    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
else()
    message(FATAL_ERROR "RETRO_SUBSYSTEM must be set to GX or SDL2")
endif()

if(NOT GAME_STATIC)
    message(FATAL_ERROR "GAME_STATIC must be on")
endif()

# pkg_check_modules(OGG ogg)
#
# if(NOT OGG_FOUND)
#     set(COMPILE_OGG TRUE)
#     message(NOTICE "libogg not found, attempting to build from source")
# else()
#     message("found libogg")
#     target_link_libraries(RetroEngine ${OGG_STATIC_LIBRARIES})
#     target_link_options(RetroEngine PRIVATE ${OGG_STATIC_LDLIBS_OTHER})
#     target_compile_options(RetroEngine PRIVATE ${OGG_STATIC_CFLAGS})
# endif()
#
# pkg_check_modules(THEORA theora theoradec)
#
# if(NOT THEORA_FOUND)
#     message("could not find libtheora, attempting to build manually")
#     set(COMPILE_THEORA TRUE)
# else()
#     message("found libtheora")
#     target_link_libraries(RetroEngine ${THEORA_STATIC_LIBRARIES})
#     target_link_options(RetroEngine PRIVATE ${THEORA_STATIC_LDLIBS_OTHER})
#     target_compile_options(RetroEngine PRIVATE ${THEORA_STATIC_CFLAGS})
# endif()

set(GAME_INCLUDE_EDITOR OFF CACHE BOOL "Disable unused editor code" FORCE)

set(SHARED_COMPILE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-threadsafe-statics>
    -Os -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fmerge-all-constants
)

set(SHARED_LINK
    -Wl,-gc-sections -Wl,--strip-all -Wl,-Map,.map
)

# Lower framebuffer size and screen count to save up RAM
set(SHARED_DEFINES
    SCREEN_XMAX=512 SCREEN_COUNT=1
)

target_compile_options(RetroEngine PRIVATE ${SHARED_COMPILE})
target_compile_definitions(RetroEngine PRIVATE ${SHARED_DEFINES} BASE_PATH="/RSDKv5/" RETRO_DISABLE_LOG=1)
target_link_options(RetroEngine PRIVATE ${SHARED_LINK})
target_link_libraries(RetroEngine fat)

target_compile_options(${GAME_NAME} PRIVATE ${SHARED_COMPILE})
target_compile_definitions(${GAME_NAME} PRIVATE ${SHARED_DEFINES})
target_link_options(${GAME_NAME} PRIVATE ${SHARED_LINK})

set(PLATFORM Wii)

ogc_create_dol(RetroEngine)
