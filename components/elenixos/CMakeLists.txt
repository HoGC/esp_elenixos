set(ELENIXOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/ElenixOS")
set(ELENIXOS_SRC_DIR "${ELENIXOS_ROOT}/src")

file(GLOB_RECURSE ELENIXOS_SOURCES
    "${ELENIXOS_SRC_DIR}/*.c"
    "${ELENIXOS_ROOT}/resources/font/*.c"
)

list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/port/critical/eos_port_critical_(bare_metal|custom|posix|rtthread)\\.c$")
list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/port/critical/eos_port_critical_freertos\\.c$")
list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/port/file_system/eos_fs_port_(posix|rtthread)\\.c$")
list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/port/memory/eos_mem_auto\\.c$")
list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/services/log/eos_log_std\\.c$")
list(FILTER ELENIXOS_SOURCES EXCLUDE REGEX ".*/lib/utils/eos_virtual_display\\.c$")

file(GLOB_RECURSE ELENIXOS_HEADERS "${ELENIXOS_SRC_DIR}/*.h")
set(ELENIXOS_INCLUDE_DIRS
    "${ELENIXOS_SRC_DIR}"
    "${ELENIXOS_ROOT}/resources"
    "${ELENIXOS_ROOT}/third_party/jerryscript/jerry-core/include"
    "${ELENIXOS_ROOT}/third_party/jerryscript/jerry-ext/include"
    "${ELENIXOS_ROOT}/third_party/cJSON"
    "${ELENIXOS_ROOT}/third_party/uthash/src"
    "${CMAKE_CURRENT_LIST_DIR}/port/include"
)

foreach(header ${ELENIXOS_HEADERS})
    get_filename_component(header_dir "${header}" DIRECTORY)
    list(APPEND ELENIXOS_INCLUDE_DIRS "${header_dir}")
endforeach()
list(REMOVE_DUPLICATES ELENIXOS_INCLUDE_DIRS)

set(ELENIXOS_PORT_SOURCES
    "port/src/audio/eos_port_audio.c"
    "port/src/battery/eos_port_battery.c"
    "port/src/critical/eos_port_critical.c"
    "port/src/eos_esp_port.c"
    "port/src/display/eos_port_display.c"
    "port/src/file_system/eos_fs_port.c"
    "port/src/log/eos_esp_log.c"
    "port/src/memory/eos_mem_port.c"
    "overlay/eos_virtual_display.c"
    "port/src/power/eos_port_power.c"
    "port/src/sensor/eos_port_sensor.c"
    "port/src/time/eos_port_time.c"
    "port/src/vibrator/eos_port_vibrator.c"
)

idf_component_register(
    SRCS
        ${ELENIXOS_SOURCES}
        ${ELENIXOS_PORT_SOURCES}
        "${ELENIXOS_ROOT}/third_party/cJSON/cJSON.c"
    INCLUDE_DIRS
        ${ELENIXOS_INCLUDE_DIRS}
    REQUIRES
        lvgl
        esp_lvgl_adapter
        esp_board_manager
        esp_lcd_touch
        esp_timer
        esp_hw_support
        spi_flash
        fatfs
        wear_levelling
        vfs
        gen_bmgr_codes
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    EOS_PLATFORM_ESP=1
    EOS_RTOS_TYPE=EOS_RTOS_FREERTOS
    EOS_MEM_ALLOC_PROVIDER=EOS_MEM_PROVIDER_CUSTOM
    EOS_FS_TYPE=EOS_FS_POSIX
    EOS_SYS_ROOT_DIR="/flash/"
    LV_FS_DEFAULT_DRIVE_LETTER=90
    LV_CONF_INCLUDE_SIMPLE
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-format
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -Wno-error=discarded-qualifiers
    -Wno-error=pointer-sign
    -Wno-error=cast-function-type
    -Wno-error=return-type
    -Wno-error=maybe-uninitialized
    -Wno-error=type-limits
    -Wno-error=unused-value
    -fgnu89-inline
)

set(JERRY_CMDLINE OFF CACHE BOOL "Build JerryScript command line tool" FORCE)
set(JERRY_CMDLINE_TEST OFF CACHE BOOL "Build JerryScript command line test tool" FORCE)
set(JERRY_CMDLINE_SNAPSHOT OFF CACHE BOOL "Build JerryScript snapshot tool" FORCE)
set(UNITTESTS OFF CACHE BOOL "Build JerryScript unit tests" FORCE)
set(DOCTESTS OFF CACHE BOOL "Build JerryScript doc tests" FORCE)
set(ENABLE_AMALGAM OFF CACHE BOOL "Enable JerryScript amalgamated build" FORCE)
set(ENABLE_LTO OFF CACHE BOOL "Enable JerryScript LTO" FORCE)
set(JERRY_ATTR_GLOBAL_HEAP "__attribute__((section(\".ext_ram.bss\")))" CACHE STRING "JerryScript heap storage attribute" FORCE)
set(JERRY_VM_HALT ON CACHE INTERNAL "Enable JerryScript VM halt feature")
set(JERRY_MEM_STATS ON CACHE INTERNAL "Enable JerryScript memory stats")
set(JERRY_LINE_INFO ON CACHE INTERNAL "Enable JerryScript line info")

add_subdirectory("${ELENIXOS_ROOT}/third_party/jerryscript" "${CMAKE_CURRENT_BINARY_DIR}/jerryscript")
if(TARGET jerry-port)
    target_sources(jerry-port PRIVATE "${CMAKE_CURRENT_LIST_DIR}/port/src/jerry/jerry_esp_port.c")
    target_include_directories(jerry-port PRIVATE "${ELENIXOS_ROOT}/third_party/jerryscript/jerry-core/include")
    target_link_libraries(jerry-port PRIVATE idf::esp_timer idf::log)
endif()
foreach(jerry_target jerry-core jerry-ext jerry-port)
    if(TARGET ${jerry_target})
        target_compile_options(${jerry_target} PRIVATE
            -Wno-error=pedantic
            -Wno-pedantic
            -Wno-error=unterminated-string-initialization
            -Wno-error=undef
        )
    endif()
endforeach()
target_link_libraries(${COMPONENT_LIB} PRIVATE jerry-core jerry-ext jerry-port)
