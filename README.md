# USBMidiWaveblaster

USBMidiWaveblaster is a program for the STM32 [Blue Pill](https://stm32-base.org/boards/STM32F103C8T6-Blue-Pill.html "Blue Pill") boards to create a USB MIDI interface for Waveblaster boards (or USB MIDI to serial MIDI adapter).

When configured with multiple USB MIDI ports, it merges MIDI messages from all ports and uses non-standard MIDI message `F5 nn` (Port selection) to switch between ports on the Waveblaster board (or on serial output).

USBMidiWaveblaster is licensed under [GNU General Public License version 3](https://www.gnu.org/licenses/gpl-3.0.html "GNU General Public License version 3").

It was adapted from the [USBMIDIKLIK 4X4](https://github.com/TheKikGen/USBMidiKliK4x4 "USBMIDIKLIK 4X4") project.

## Compiling/Installation

You need [Arduino IDE (1.8.x)](https://www.arduino.cc/en/software "Arduino IDE (1.8.x)") with STM32duino and a STM32 [Blue Pill](https://stm32-base.org/boards/STM32F103C8T6-Blue-Pill.html "Blue Pill") board.
Installation instructions for STM32duino are available on these wiki pages: [https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki](https://github.com/rogerclarkmelbourne/Arduino_STM32/wiki).

Open the file `USBMidiWaveblaster.ino` in Arduino IDE.

Choose following settings in the "Tools" menu:

* Board: "Generic STM32F103C series"
* Variant: "STM32F103C8 (20k RAM. 64k Flash)" or "STM32F103CB (20k RAM. 128k Flash)" depending on your Blue Pill version
* CPU Speed(MHz): "72 Mhz (Normal)"
* Optimize: "Fastest -O3"
* Upload method: As you want/need. You can also add new uploading methods, i.e. [TKG HID bootloader](https://github.com/TheKikGen/stm32-tkg-hid-bootloader "TKG HID bootloader")

You can adjust the configuration to your needs in file `config.h`.

Connect the Bluepill to your computer and compile and upload the program.
