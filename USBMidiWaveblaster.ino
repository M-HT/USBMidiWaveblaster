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
  This file uses code from "USBMIDIKLIK 4X4" by "The KikGen Labs".
  https://github.com/TheKikGen/USBMidiKliK4x4/
  ----------------------------------------------------------------------
  MAIN SOURCE
  ----------------------------------------------------------------------

*/

#include "hardware_config.h"
#include "usb_midi.h"
#include "usb_midi_device.h"
#include "config.h"

#define LED_FLASH_TIME 5
#define LED_IDLE_TIME  500

typedef union  {
    uint32_t i;
    uint8_t  packet[4];
} __packed midiPacket_t;

// Serial interfaces Array
HardwareSerial * serialHw[SERIAL_INTERFACE_MAX] = {SERIALS_PLIST};
uint32_t serialSpeed[SERIAL_INTERFACE_MAX];

// USB Midi object & globals
USBMidi MidiUSB;
bool midiUSBCx    = false;
bool isSerialBusy = false;

bool ledStatus;

uint8_t runningStatus = 0;
uint8_t lastPort = 0xFF;
uint8_t portSelection[2] = { 0xF5, 0x01 };

// Write data to all enabled serial ports
void SerialWrite(uint8_t *data, uint8_t len)
{
    for ( uint8_t s = 0; s < SERIAL_INTERFACE_MAX ; s++ )
    {
        if ( serialSpeed[s] == 0 ) continue;

        serialHw[s]->write(data, len);
    }
}

// Process MIDI 1.0 packet
void ProcessPacket(midiPacket_t *pk)
{
    uint8_t port = pk->packet[0] >> 4;
    uint8_t cin  = pk->packet[0] & 0x0F;

#if USB_MIDI_IO_PORT_NUM < 16
    if (port >= USB_MIDI_IO_PORT_NUM)
    {
        // Ignore packets from unused ports
        return;
    }
#endif

    uint8_t msgLen = USBMidi::CINToLenTable[cin];

#if USB_MIDI_IO_PORT_NUM >= 2
    switch ( cin )
    {
        case 0x02: // Two-byte System Common messages like MTC, SongSelect, etc.
        case 0x03: // Three-byte System Common messages like SPP, etc.
        case 0x05: // Single-byte System Common Message or SysEx ends with following single byte.
        case 0x06: // SysEx ends with following two bytes.
        case 0x07: // SysEx ends with following three bytes.
            // Ignore Port Selection messages "F5 nn" or other non-standard "F5" messages (1-3 bytes)
            if ( pk->packet[1] == 0xF5 ) msgLen = 0;
            break;
        case 0x0F: // Single Byte
            // Only allow System RealTime messages
            if ( pk->packet[1] < 0xF8 ) msgLen = 0;
            break;
        default:
            break;
    }
#endif

    if ( msgLen > 0 )
    {
#if USB_MIDI_IO_PORT_NUM >= 2
        // If last message came from different port, then send Port Selection message "F5 nn"
        if ( port != lastPort )
        {
            runningStatus = 0;
            lastPort = port;
            portSelection[1] = port + 1;
            SerialWrite(portSelection, 2);
        }
#endif

#if defined(CFG_SERIAL_RUNNING_STATUS) && CFG_SERIAL_RUNNING_STATUS > 0
        // Implement Running Status when sending data to maximize available bandwidth
        if (pk->packet[1] >= 0xF8)
        {
            // RealTime messages
            SerialWrite(&pk->packet[1], msgLen);
        }
        else if (pk->packet[1] >= 0xF0)
        {
            // System Common messages
            runningStatus = 0;
            SerialWrite(&pk->packet[1], msgLen);
        }
        else if (pk->packet[1] >= 0x80)
        {
            if (pk->packet[1] == runningStatus)
            {
                // Don't send Running Status byte
                if (msgLen > 1)
                {
                    SerialWrite(&pk->packet[2], msgLen - 1);
                }
            }
            else
            {
                // Update Running Status
                runningStatus = pk->packet[1];
                SerialWrite(&pk->packet[1], msgLen);
            }
        }
        else
#endif
        {
            SerialWrite(&pk->packet[1], msgLen);
        }
    }
}

// Turn LED on
void LED_TurnOn(void)
{
    if (!ledStatus)
    {
        ledStatus = true;
        digitalWrite(LED_CONNECT, LOW);
    }
}

// Turn LED off
void LED_TurnOff(void)
{
    if (ledStatus)
    {
        ledStatus = false;
        digitalWrite(LED_CONNECT, HIGH);
    }
}

void setup()
{
    Serial.end();

    // Initialize LED pin as an output
    pinMode(LED_CONNECT, OUTPUT);
    ledStatus = false;
    digitalWrite(LED_CONNECT, HIGH);

    // Prepare serial ports speed
    for ( uint8_t s=0; s != SERIAL_INTERFACE_MAX ; s++ ) serialSpeed[s] = 0;

#if SERIAL_INTERFACE_MAX >= 1 && defined(CFG_SERIAL_PORT_1_SPEED)
    serialSpeed[0] = CFG_SERIAL_PORT_1_SPEED;
#endif
#if SERIAL_INTERFACE_MAX >= 2 && defined(CFG_SERIAL_PORT_2_SPEED)
    serialSpeed[1] = CFG_SERIAL_PORT_2_SPEED;
#endif
#if SERIAL_INTERFACE_MAX >= 3 && defined(CFG_SERIAL_PORT_3_SPEED)
    serialSpeed[2] = CFG_SERIAL_PORT_3_SPEED;
#endif
#if SERIAL_INTERFACE_MAX >= 4 && defined(CFG_SERIAL_PORT_4_SPEED)
    serialSpeed[3] = CFG_SERIAL_PORT_4_SPEED;
#endif

    // MIDI SERIAL PORTS set Baud rates and parser inits
    // To compile with the 4 serial ports, you must use the right variant : STMF103RC

    for ( uint8_t s=0; s != SERIAL_INTERFACE_MAX ; s++ )
    {
        if ( serialSpeed[s] == 0 ) continue;

        serialHw[s]->begin(serialSpeed[s]);
    }

    // Configure USB MIDI parameters
#if defined(CFG_USB_MIDI_VENDORID) && (CFG_USB_MIDI_PRODUCTID)
    usb_midi_set_vid_pid(CFG_USB_MIDI_VENDORID, CFG_USB_MIDI_PRODUCTID);
#endif
#ifdef CFG_USB_MIDI_PRODUCT_STRING
    usb_midi_set_product_string(CFG_USB_MIDI_PRODUCT_STRING);
#endif
#ifdef CFG_USB_MIDI_JACK_STRING
    usb_midi_set_jack_string(CFG_USB_MIDI_JACK_STRING);
#endif

    MidiUSB.begin() ;
    while (! MidiUSB.isConnected() ) delay(100); // Usually around 4 s to detect USB Midi on the host
}

void loop()
{
    static unsigned long turnOnMillis = 0;
    static unsigned long turnOffMillis = 0;
    static bool turnOffEnabled = false;
    unsigned long currentMillis = millis();

    // Process incoming USB packets
    if ( MidiUSB.isConnected() )
    {
        // Turn LED off after flash timeout
        if (turnOffEnabled && currentMillis > turnOffMillis)
        {
            turnOffEnabled = false;
            LED_TurnOff();
        }

        // Turn on LED after idle timeout
        if (currentMillis > turnOnMillis)
        {
            turnOnMillis = currentMillis + LED_IDLE_TIME;
            LED_TurnOn();
        }

        midiUSBCx = true;

        // Do we have a MIDI USB packet available ?
        if ( MidiUSB.available() )
        {
            // Set idle timeout
            turnOnMillis = currentMillis + LED_IDLE_TIME;

            // Read a Midi USB packet .
            if ( !isSerialBusy )
            {
                midiPacket_t pk;
                pk.i = MidiUSB.readPacket();

                // Turn LED on and set flash timeout
                turnOffMillis = currentMillis + LED_FLASH_TIME;
                turnOffEnabled = true;
                LED_TurnOn();

                ProcessPacket(&pk);
            }
            else
            {
                isSerialBusy = false;
            }
        }
    }
    // Are we physically connected to USB
    else
    {
        runningStatus = 0;
        lastPort = 0xFF;

        // Turn LED off
        turnOffEnabled = false;
        LED_TurnOff();

        midiUSBCx = false;
    }


    // Process Serial ports
    for ( uint8_t s = 0; s < SERIAL_INTERFACE_MAX ; s++ )
    {
        if ( serialSpeed[s] == 0 ) continue;

        // Do we have any MIDI msg on Serial 1 to n ?
        if ( serialHw[s]->available() )
        {
            serialHw[s]->read();
        }

        // Manage Serial contention vs USB
        // When one or more of the serial buffer is full, we block USB read one round.
        // This implies to use non blocking Serial.write(buff,len).
        if (  midiUSBCx &&  !serialHw[s]->availableForWrite() ) isSerialBusy = true; // 1 round without reading USB
    }
}
