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

  ----------------------------------------------------------------------
  This file was adapted from "USBMIDIKLIK 4X4" by "The KikGen Labs".
  https://github.com/TheKikGen/USBMidiKliK4x4/
  ----------------------------------------------------------------------
  USB MIDI LIBRARY adapted by TheKikGenLab from USB LeafLabs LLC. USB API :
  Perry Hung, Magnus Lundin,Donald Delmar Davis, Suspect Devices.
  ----------------------------------------------------------------------

*/

#pragma once

/**
 * @brief Wirish USB MIDI port (MidiUSB).
 */

#ifndef _WIRISH_USB_MIDI_H_
#define _WIRISH_USB_MIDI_H_

#define USB_MIDI
#define USB_HARDWARE

#include <Print.h>
#include <boards.h>


class USBMidi {
private:

public:
    // Len of packets. Direct access allowed.
    static const uint8_t CINToLenTable[16];
    // Constructor
    USBMidi();

    void begin();
    void end();
    uint32_t available(void);
    bool   isTransmitting(void);
    uint32_t readPackets(const void *buf, uint32_t len);
    uint32_t readPacket();
    uint32_t peekPacket();
    void   markPacketRead();
    void   writePacket(const uint32*);
    void   writePackets(const void*, uint32);
    uint8_t  isConnected();
    uint8_t  pending();
 };

#endif


