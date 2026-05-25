# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_elenixos)

set(ELENIXOS_SIM_FS_DIR "${CMAKE_CURRENT_LIST_DIR}/fs")
set(ELENIXOS_FS_ROOT "${CMAKE_BINARY_DIR}/elenixos_fs_root")

add_custom_target(elenixos_fs_root
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${ELENIXOS_FS_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${ELENIXOS_FS_ROOT}/.sys/config"
        "${ELENIXOS_FS_ROOT}/.sys/res/img"
        "${ELENIXOS_FS_ROOT}/.sys/app"
        "${ELENIXOS_FS_ROOT}/.sys/wf"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ELENIXOS_SIM_FS_DIR}/.sys/config"
        "${ELENIXOS_FS_ROOT}/.sys/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ELENIXOS_SIM_FS_DIR}/.sys/res/img"
        "${ELENIXOS_FS_ROOT}/.sys/res/img"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ELENIXOS_SIM_FS_DIR}/.sys/app"
        "${ELENIXOS_FS_ROOT}/.sys/app"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ELENIXOS_SIM_FS_DIR}/.sys/wf"
        "${ELENIXOS_FS_ROOT}/.sys/wf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ELENIXOS_SIM_FS_DIR}/.sys/state.json"
        "${ELENIXOS_FS_ROOT}/.sys/state.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ELENIXOS_SIM_FS_DIR}/my_app.eapk"
        "${ELENIXOS_FS_ROOT}/my_app.eapk"
    COMMENT "Preparing ElenixOS flash filesystem root without bundled TTF fonts"
)

fatfs_create_spiflash_image(storage "${ELENIXOS_FS_ROOT}" FLASH_IN_PROJECT)
add_dependencies(fatfs_storage_bin elenixos_fs_root)
