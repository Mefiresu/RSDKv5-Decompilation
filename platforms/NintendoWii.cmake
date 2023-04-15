find_package(PkgConfig REQUIRED)

add_executable(RetroEngine ${RETRO_FILES})

set(RETRO_SUBSYSTEM "GX" CACHE STRING "The subsystem to use")
if(NOT (RETRO_SUBSYSTEM STREQUAL "GX"))
    message(FATAL_ERROR "RETRO_SUBSYSTEM must be set to GX")
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

set(SHARED_COMPILE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-threadsafe-statics> 
    -Os -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fmerge-all-constants
)

set (SHARED_LINK
    -Wl,-gc-sections -Wl,--strip-all -Wl,--orphan-handling=discard -Wl,-Map,.map
)

target_compile_options(RetroEngine PRIVATE ${SHARED_COMPILE})
target_compile_definitions(RetroEngine PRIVATE BASE_PATH="/RSDKv5/" RETRO_DISABLE_LOG=1)
target_link_options(RetroEngine PRIVATE ${SHARED_LINK})
target_link_libraries(RetroEngine fat aesnd)

target_compile_options(${GAME_NAME} PRIVATE ${SHARED_COMPILE})
target_compile_definitions(${GAME_NAME} PRIVATE RETRO_INCLUDE_EDITOR=0)
target_link_options(${GAME_NAME} PRIVATE ${SHARED_LINK})

set(PLATFORM Wii)

ogc_create_dol(RetroEngine)
