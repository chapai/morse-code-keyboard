# Flipper | Morse Code Keyboard
Flipper app to simulate hid device for morse code input. 

This project is aimed to put my grandpa's morse key to use and make it somehow useful. The idea is to convert morse key to keyboard input so it can be used with PC. This is probably not the most eficient way to use your computer but definetly a fun one.

## Milestones
- Morse code tree and simple navigation
- Basic GUI
- USB HID interface

 ### ----- You are here -----

- Extended morse protocol (keyboard-specific symbols)
- Bluetooth support
- GPI Support and hardware

## How to run
Build the firmware

``` ./fbt ```

Build the fap

``` ./fbt fap_morsecodeapp ```

Run 

``` ./fbt launch_app APPSRC=applications_user/morse_code_keyboard ```
