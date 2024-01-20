/**
  Copyright (C) 2017-2018  The KikGen Labs
  Copyright (C) 2024  Roman Pauer

  This file is part of USBMidiWaveblaster.
  https://github.com/M-HT/USBMidiWaveblaster/

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#ifndef _CONFIG_H_
#define _CONFIG_H_
#pragma once

// Uncomment to change the number of USD MIDI ports
//#define CFG_USB_MIDI_IO_PORT_NUM         1

// Uncomment to change the USB Vendor and Product ID
//#define CFG_USB_MIDI_VENDORID            0xF055
//#define CFG_USB_MIDI_PRODUCTID           0x5742

// Uncomment to change the USB Product Name (maximum length 30, only ASCII characters)
//#define CFG_USB_MIDI_PRODUCT_STRING      "Waveblaster"

// Uncomment to change the USB Port Name (maximum length 11, only ASCII characters)
//#define CFG_USB_MIDI_JACK_STRING         "Waveblaster"

// Uncomment to change the USB device to low power (100 mA)
//#define CFG_USB_MIDI_LOW_POWER           1

// Comment to disable Running Status on serial ports (i.e. always send complete MIDI message)
#define CFG_SERIAL_RUNNING_STATUS        1

// Uncomment/comment to enable/disable serial ports and change the speed (bauds)
//#define CFG_SERIAL_PORT_1_SPEED 38400
#define CFG_SERIAL_PORT_2_SPEED 31250
//#define CFG_SERIAL_PORT_3_SPEED 115200
//#define CFG_SERIAL_PORT_4_SPEED 57600

#endif
