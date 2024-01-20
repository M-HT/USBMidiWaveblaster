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
  SOME HARDWARE CONFIGURATION
  ----------------------------------------------------------------------

*/

#ifndef _HARDWARE_CONFIG_H_
#define _HARDWARE_CONFIG_H_
#pragma once

#ifdef MCU_STM32F103RC
  #warning "MIDITECH OR MCU_STM32F103RC HARDWARE DETECTED"

  // Comment the line below for a generic STM32F103RC
  // This drives the DISC pin for USB with the Miditech 4x4
  // and the connect LED pin #.
  // Activated by default.
  #define HAS_MIDITECH_HARDWARE

  #define SERIAL_INTERFACE_MAX  4
  #define SERIALS_PLIST &Serial1,&Serial2,&Serial3,&Serial4
  #ifdef HAS_MIDITECH_HARDWARE
     #warning "MIDITECH4X4 STM32F103RC HARDWARE DETECTED"
     #define HARDWARE_TYPE "MIDITECH4x4 STM32F103RC"
     #define LED_CONNECT PC9
  #else
     #warning "STM32F103RC HARDWARE DETECTED"
     #define HARDWARE_TYPE "STM32F103RC"
     #define LED_CONNECT PC13
  #endif

#else
  #if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB)
    #warning "BLUEPILL HARDWARE DETECTED"
    #define HARDWARE_TYPE "BLUEPILL STMF103C8x"
    #define SERIAL_INTERFACE_MAX  3
    #define SERIALS_PLIST &Serial1,&Serial2,&Serial3
    #define LED_CONNECT PC13
  #else
   #error "PLEASE CHOOSE STM32F103RC (4 serial ports) or STM32F103RC (3 serial ports) variants to compile ."
  #endif
#endif

#endif


