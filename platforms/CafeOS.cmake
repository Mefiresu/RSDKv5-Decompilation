find_package(PkgConfig REQUIRED)

add_executable(RetroEngine ${RETRO_FILES})


set(RETRO_SUBSYSTEM "OGL" CACHE STRING "The subsystem to use")

pkg_check_modules(OGG ogg)

if(NOT OGG_FOUND)
    set(COMPILE_OGG TRUE)
    message(NOTICE "libogg not found, attempting to build from source")
else()
    message("found libogg")
    target_link_libraries(RetroEngine ${OGG_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${OGG_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${OGG_STATIC_CFLAGS})
endif()

pkg_check_modules(THEORA theora theoradec)

if(NOT THEORA_FOUND)
    message("could not find libtheora, attempting to build manually")
    set(COMPILE_THEORA TRUE)
else()
    message("found libtheora")
    target_link_libraries(RetroEngine ${THEORA_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${THEORA_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${THEORA_STATIC_CFLAGS})
endif()


if(RETRO_SUBSYSTEM STREQUAL "OGL")
    target_link_libraries(RetroEngine "-lGLESv2 -lANGLE -lz -lEGL")
    target_include_directories(RetroEngine PRIVATE
        /opt/devkitpro/portlibs/wiiu/include/angle
    )

    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
elseif(RETRO_SUBSYSTEM STREQUAL "SDL2")
    pkg_check_modules(SDL2 sdl2 REQUIRED)
    target_link_libraries(RetroEngine ${SDL2_STATIC_LIBRARIES})
    target_link_options(RetroEngine PRIVATE ${SDL2_STATIC_LDLIBS_OTHER})
    target_compile_options(RetroEngine PRIVATE ${SDL2_STATIC_CFLAGS})
endif()

wut_create_rpx(RetroEngine)

set(PLATFORM CafeOS)