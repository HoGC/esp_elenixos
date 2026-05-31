# esp_elenixos

English | [中文](./README.zh-CN.md)

`esp_elenixos` is the ESP-IDF port of ElenixOS, used to run the ElenixOS smartwatch interface on the ESP32-S31 Korvo1 board.

ElenixOS is an open-source smartwatch operating system. It builds its graphical interface with LVGL, while watch faces and applications run in a JerryScript-powered scripting runtime. It targets resource-constrained embedded devices, with a focus on portability, extensibility, gestures, animations, and a smartwatch-like application experience.

## Online Demo

You can try the ElenixOS online simulator directly in your browser without preparing any hardware:

[https://simulator.elenixos.com/wasm/latest/main.html](https://simulator.elenixos.com/wasm/latest/main.html)

## Demonstration Image

<img src="./_static/esp32-s31-korvo1.jpg" alt="ESP32-S31 Korvo1 running ElenixOS" width="640">

## Project Notes

- The ESP-IDF project entry is `CMakeLists.txt`.
- The main application is in `main/main.c`. It initializes board peripherals, LCD, touch, the LVGL adapter, and starts ElenixOS.
- The ElenixOS core and ESP port layer are in `components/elenixos/`.
- The LVGL component is in `components/lvgl/`.
- `fs/` is packaged as the `storage` FAT partition image during build and mounted as the ElenixOS `/flash` system root.
- The current default target is `esp32s31`, and the board configuration is `esp32_s31_korvo1`.

## Build And Flash

Enter the ESP-IDF environment first:

```sh
. $IDF_PATH/export.sh
```

Then run the following commands in this directory:

```sh
pip install esp-bmgr-assist
idf.py bmgr -b esp32_s31_korvo1
idf.py set-target esp32s31
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

If your serial port is different, replace `/dev/ttyUSB0` with the actual device path.

## Links

- ElenixOS documentation: https://docs.elenixos.com
- ElenixOS repository: https://github.com/ElenixOS/ElenixOS
- LVGL: https://lvgl.io
