/*
  USB MidiKliK 4X4 - USB MIDI 4 IN X 4 OUT firmware
  Based on the MIDITECH / MIDIPLUS 4X4 harware.
  Copyright (C) 2017/2018 by The KikGen labs.

  MAIN SOURCE

  ------------------------   CAUTION  ----------------------------------
  THIS NOT A COPY OR A HACK OF ANY EXISTING MIDITECH/MIDIPLUS FIRMWARE.
  THAT FIRMWARE WAS ENTIRELY CREATED FROM A WHITE PAGE, WITHOUT
  DISASSEMBLING ANY SOFTWARE FROM MIDITECH/MIDIPLUS.

  UPLOADING THIS FIRMWARE TO YOUR MIDIPLUS/MIDITECH 4X4 USB MIDI
  INTERFACE  WILL PROBABLY CANCEL YOUR WARRANTY.

  IT WILL NOT BE POSSIBLE ANYMORE TO UPGRADE THE MODIFIED INTERFACE
  WITH THE MIDITECH/MIDIPLUS TOOLS AND PROCEDURES. NO ROLLBACK.

  THE AUTHOR DISCLAIM ANY DAMAGES RESULTING OF MODIFYING YOUR INTERFACE.
  YOU DO IT AT YOUR OWN RISKS.
  ---------------------------------------------------------------------

  This file is part of the USBMIDIKLIK-4x4 distribution
  https://github.com/TheKikGen/USBMidiKliK4x4
  Copyright (c) 2018 TheKikGen Labs team.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "hardware_config.h"
#include "usb_midi.h"
#include "usb_midi_device.h"

enum MidiRouteSourceDest {
  FROM_SERIAL,
  FROM_USB,
  TO_SERIAL,
  TO_USB,
} ;

// Use this structure to send and receive packet to/from USB /serial/BUS
typedef union  {
    uint32_t i;
    uint8_t  packet[4];
} __packed midiPacket_t;

// Serial interfaces Array
HardwareSerial * serialHw[SERIAL_INTERFACE_MAX] = {SERIALS_PLIST};

// USB Midi object & globals
USBMidi MidiUSB;
volatile bool					midiUSBCx      = false;
bool 					isSerialBusy   = false ;

/////////////////////////////// END GLOBALS ///////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Send a USB midi packet to ad-hoc serial MIDI
///////////////////////////////////////////////////////////////////////////////
static void SerialMidi_SendPacket(const midiPacket_t *pk, uint8_t serialNo)
{
  if (serialNo >= SERIAL_INTERFACE_MAX ) return;

	uint8_t msgLen = USBMidi::CINToLenTable[pk->packet[0] & 0x0F] ;
 	if ( msgLen > 0 ) {
		serialHw[serialNo]->write(&(pk->packet[1]),msgLen);
	}
}

///////////////////////////////////////////////////////////////////////////////
// THE MIDI PACKET ROUTER
//-----------------------------------------------------------------------------
// Route a packet from a midi IN jack / USB OUT to
// a midi OUT jacks / USB IN  or I2C remote serial midi on another device
///////////////////////////////////////////////////////////////////////////////
static void RoutePacketToTarget(uint8_t source, const midiPacket_t *pk)
{
  // NB : we use the same routine to route USB and serial/ I2C .
	// The Cable can be the serial port # if coming from local serial
  uint8_t port  = pk->packet[0] >> 4;

	// Check at the physical level (i.e. not the bus)
  if ( source == FROM_USB && port >= USBCABLE_INTERFACE_MAX ) return;

  uint8_t cin   = pk->packet[0] & 0x0F ;

	// ROUTING tables
	uint16_t serialOutTargets ;

  if (source == FROM_USB ) {
      serialOutTargets = 1 << port;
  }

  else return; // Error.


	// ROUTING FROM ANY SOURCE PORT TO SERIAL TARGETS //////////////////////////
	// A target match ?
  if ( serialOutTargets) {
				for (	uint16_t t=0; t<SERIAL_INTERFACE_MAX ; t++)
					if ( (serialOutTargets & ( 1 << t ) ) ) {
								// Route to local serial if bus mode disabled
								SerialMidi_SendPacket(pk,t);
					}
	} // serialOutTargets

  // Stop here if no USB connection (owned by the master).
  // If we are a slave, the master should have notified us
  if ( ! midiUSBCx ) return;

}

///////////////////////////////////////////////////////////////////////////////
// MIDI USB initiate connection if master
// + Set USB descriptor strings
///////////////////////////////////////////////////////////////////////////////
void USBMidi_Init()
{
	MidiUSB.begin() ;
  delay(4000); // Usually around 4 s to detect USB Midi on the host
}

///////////////////////////////////////////////////////////////////////////////
// MIDI USB Loop Process
///////////////////////////////////////////////////////////////////////////////
void USBMidi_Process()
{
	// Try to connect/reconnect USB if we detect a high level on USBDM
	// This is to manage the case of a powered device without USB active or suspend mode for ex.
	if ( MidiUSB.isConnected() ) {

    midiUSBCx = true;

		// Do we have a MIDI USB packet available ?
		if ( MidiUSB.available() ) {

			// Read a Midi USB packet .
			if ( !isSerialBusy ) {
				midiPacket_t pk ;
				pk.i = MidiUSB.readPacket();
				RoutePacketToTarget( FROM_USB,  &pk );
			} else {
					isSerialBusy = false ;
			}
		}
	}
	// Are we physically connected to USB
	else {
       midiUSBCx = false;
  }
}

///////////////////////////////////////////////////////////////////////////////
// I2C Loop Process for SERIAL MIDI
///////////////////////////////////////////////////////////////////////////////
void SerialMidi_Process()
{
	// LOCAL SERIAL JACK MIDI IN PROCESS
	for ( uint8_t s = 0; s< SERIAL_INTERFACE_MAX  ; s++ )
	{
				// Do we have any MIDI msg on Serial 1 to n ?
				if ( serialHw[s]->available() ) {
					 serialHw[s]->read();
				}

				// Manage Serial contention vs USB
				// When one or more of the serial buffer is full, we block USB read one round.
				// This implies to use non blocking Serial.write(buff,len).
				if (  midiUSBCx &&  !serialHw[s]->availableForWrite() ) isSerialBusy = true; // 1 round without reading USB
	}
}

///////////////////////////////////////////////////////////////////////////////
// SETUP
///////////////////////////////////////////////////////////////////////////////
void setup()
{

    // MIDI MODE START HERE ==================================================

    // MIDI SERIAL PORTS set Baud rates and parser inits
    // To compile with the 4 serial ports, you must use the right variant : STMF103RC
    // + Set parsers filters in the same loop.  All messages including on the fly SYSEX.

    for ( uint8_t s=0; s < SERIAL_INTERFACE_MAX ; s++ ) {
      serialHw[s]->begin(31250);
    }

    // Midi USB only if master when bus is enabled or master/slave
        USBMidi_Init();

}

///////////////////////////////////////////////////////////////////////////////
// LOOP
///////////////////////////////////////////////////////////////////////////////
void loop()
{
		USBMidi_Process();

		SerialMidi_Process();
}
