# usb_midi_pedalx2
USB MIDI soft and sustain pedals sending continuous controller values.

Using the STM32F103 (Bluepill) this project provides a USB MIDI device with two continuous pedals, soft (CC67) and sustain (CC64). It measures the voltage at two of its ADC pins, applies endstop dead zones and noise filtering to give continuous control instead of the more nomal on/off toggle. Some pianos (such as Modartt Pianoteq) support such continous pedal control. The pedal range is automatically calibrated when a pedal is pressed. There is also a serial communication port provided for debug information.

There is a PlatformIO project file included to ease development and deploy. The Maple Arduino framework is used which provides composite USB support, including USB MIDI.

## Build and Installation

It _should_ be as simple as opening the project in PlatformIO, build and upload. This assumes the STM32F103 board has the maple bootloader installed at 0x0800000 which can be written using STLINK. I used a precompiled version from [Roger Clark's GitHub repository](https://github.com/rogerclarkmelbourne/STM32duino-bootloader). I use VSCode as the development tool with the PlatformIO plugin installed. There are some modifications required to make this work which are detailed in a [blog post](https://github.com/riban-bw/blog/wiki/STM32--development-on-PlatformIO-in-VSCode-on-64-bi-ARM).

## Hardware

I use AH49E Hall effect sensors with 3.3V supply and the output pin connected directly to the STM32F103 ADC pin. A 5mm x 2mm Neodymium magnet is attacehed to the pedal that swipes the magent past the sensor. A similar effect could be obtained (possibly with less accurate positioning required) by moving the magnet linearly towards the sensor. It could also be done with a potentiometer.

## Use

Plug in the device to a PC or similar that supports class compliant USB MIDI devices. A MIDI device should appear (with both MIDI input and output, though the output is not used) which will provide CC messages for each pedal.

## Debug

There is also a serial port presented in the USB interface which may be accessed with a terminal application to view debug output.

## Updating

It should be possible to upload new firmware to the device using the DFU upload protocol. If this fails, it may be possible to insert the USB device whilst the upload program is searching for DFU port.
