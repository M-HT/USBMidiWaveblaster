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

#include <string.h>
#include <libmaple/nvic.h>
#include <EEPROM.h>
#include <Wire_slave.h>
#include <PulseOutManager.h>
#include <midiXparser.h>
#include "build_number_defines.h"
#include "hardware_config.h"
#include "UsbMidiKliK4x4.h"
#include "usb_midi.h"
#include "usb_midi_device.h"
#include "EEPROM_Params.h"
#include "RingBuffer.h"

// EEPROMS parameters
EEPROM_Params_t EEPROM_Params;

// Timer
HardwareTimer timer(2);

// Serial interfaces Array
HardwareSerial * serialHw[SERIAL_INTERFACE_MAX] = {SERIALS_PLIST};

// Prepare LEDs pulse for Connect, MIDIN and MIDIOUT
// From MIDI SERIAL point of view
// Use a PulseOutManager factory to create the pulses
PulseOutManager flashLEDManager;

PulseOut* flashLED_CONNECT = flashLEDManager.factory(LED_CONNECT,LED_PULSE_MILLIS,LOW);

#ifdef HAS_MIDITECH_HARDWARE

  // LED must be declared in the same order as hardware serials
  #define LEDS_MIDI
  PulseOut* flashLED_IN[SERIAL_INTERFACE_MAX] =
	{
    flashLEDManager.factory(D4,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D5,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D6,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D7,LED_PULSE_MILLIS,LOW)
  };

  PulseOut* flashLED_OUT[SERIAL_INTERFACE_MAX] =
	{
    flashLEDManager.factory(D36,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D37,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D16,LED_PULSE_MILLIS,LOW),
    flashLEDManager.factory(D17,LED_PULSE_MILLIS,LOW)
  };

#endif

// Timer used to signal I2C events every 300 ms
PulseOut I2C_LedTimer(0xFF,500);


// Timer used to display debug msg and avoid com overflow
#ifdef DEBUG_MODE
PulseOut I2C_DebugTimer(0xFF,1000);
#endif

// USB Midi object & globals
USBMidi MidiUSB;
volatile bool					midiUSBCx      = false;
volatile bool         midiUSBIdle    = false;
bool          midiUSBLaunched= false;
bool 					isSerialBusy   = false ;
unsigned long midiUSBLastPacketMillis    = 0;

// MIDI Parsers for serial 1 to n
midiXparser midiSerial[SERIAL_INTERFACE_MAX];

// Sysex used to set some parameters of the interface.
// Be aware that the 0x77 manufacturer id is reserved in the MIDI standard (but not used)
// The second byte is usually an id number or a func code + the midi channel (any here)
// The Third is the product id
static  const uint8_t sysExInternalHeader[] = {SYSEX_INTERNAL_HEADER} ;
static  uint8_t sysExInternalBuffer[SYSEX_INTERNAL_BUFF_SIZE] ;

// Intelligent midi thru mode
volatile bool intelliThruActive = false;
unsigned long intelliThruDelayMillis = DEFAULT_INTELLIGENT_MIDI_THRU_DELAY_PERIOD * 15000;

// Bus Mode globals
boolean I2C_DeviceActive[B_MAX_NB_DEVICE-1]; // Minus the master
boolean I2C_MasterReady = false;
volatile unsigned long I2C_MasterReadyTimeoutMillis = 0;
volatile uint8_t I2C_Command = B_CMD_NONE;

// Master to slave synchonization globals
volatile boolean I2C_SlaveSyncStarted = false;
volatile boolean I2C_SlaveSyncDoUpdate = false;

// Templated RingBuffers to manage I2C slave reception/transmission outside I2C ISR
// Volatile by default and RESERVED TO SLAVE
RingBuffer<uint8_t,B_RING_BUFFER_PACKET_SIZE> I2C_QPacketsFromMaster;
RingBuffer<uint8_t,B_RING_BUFFER_MPACKET_SIZE> I2C_QPacketsToMaster;

/////////////////////////////// END GLOBALS ///////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Timer2 interrupt handler
///////////////////////////////////////////////////////////////////////////////
void Timer2Handler(void)
{
     // Update LEDS & timer
     flashLEDManager.update(millis());
     I2C_LedTimer.update(millis());
     #ifdef DEBUG_MODE
     I2C_DebugTimer.update(millis());
     #endif
}

///////////////////////////////////////////////////////////////////////////////
// FlashAllLeds . 0 = Alls. 1 = In. 2 = Out
///////////////////////////////////////////////////////////////////////////////
void FlashAllLeds(uint8_t mode)
{
	for ( uint8_t f=0 ; f< 4 ; f++ ) {
		#ifdef LEDS_MIDI
			for ( uint8_t i=0 ; i< SERIAL_INTERFACE_MAX ; i++ ) {
					if ( mode == 0 || mode ==1 ) flashLED_IN[i]->start();
					if ( mode == 0 || mode ==2 ) 	flashLED_OUT[i]->start();
			}
		#else
			flashLED_CONNECT->start();
		#endif

			delay(100);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Send a midi msg to serial MIDI. 0 is Serial1.
///////////////////////////////////////////////////////////////////////////////
static void SerialMidi_SendMsg(uint8_t const *msg, uint8_t serialNo)
{
  if (serialNo >= SERIAL_INTERFACE_MAX ) return;

	uint8_t msgLen = midiXparser::getMidiStatusMsgLen(msg[0]);

	if ( msgLen > 0 ) {
	  serialHw[serialNo]->write(msg,msgLen);
		FLASH_LED_OUT(serialNo);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Send a USB midi packet to ad-hoc serial MIDI
///////////////////////////////////////////////////////////////////////////////
static void SerialMidi_SendPacket(const midiPacket_t *pk, uint8_t serialNo)
{
  if (serialNo >= SERIAL_INTERFACE_MAX ) return;

	uint8_t msgLen = USBMidi::CINToLenTable[pk->packet[0] & 0x0F] ;
 	if ( msgLen > 0 ) {
		serialHw[serialNo]->write(&(pk->packet[1]),msgLen);
		FLASH_LED_OUT(serialNo);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Send a midi packet to I2C remote MIDI device on BUS
///////////////////////////////////////////////////////////////////////////////
static void I2C_BusSerialSendMidiPacket(const midiPacket_t *pk, uint8_t targetPort)
{
	if ( EEPROM_Params.I2C_BusModeState == B_DISABLED) return;

	// Check if it is a local port to avoid bus
	// bus traffic for Nothing
	if ( EEPROM_Params.I2C_DeviceId == GET_DEVICEID_FROM_SERIALNO(targetPort) ) {
		SerialMidi_SendPacket(pk, targetPort % SERIAL_INTERFACE_MAX );

DEBUG_BEGIN
DEBUG_PRINTLN("PK local sent to LOCAL SERIAL","");
DEBUG_END

		return;
	}

	// If we are a slave, we can't talk directly to the bus.
	// So, store the "to transmit" packet, waiting for a RequestFrom.
  // Master packet have  a "dest" byte before the midi packet.
  if ( EEPROM_Params.I2C_DeviceId != B_MASTERID ) {
    masterMidiPacket_t mpk;
    mpk.mpk.dest = TO_SERIAL;
    mpk.mpk.pk.i = pk->i;// Copy the midi packet and queue it

		I2C_QPacketsToMaster.write(mpk.packet,sizeof(masterMidiPacket_t));

DEBUG_BEGIN
DEBUG_PRINTLN("PK dest SERIAL to master Q.count=",I2C_QPacketsToMaster.available());
DEBUG_END

		return;
	}

	// We are a MASTER !
	// Compute the device ID from the serial port Id
	uint8_t deviceId = GET_DEVICEID_FROM_SERIALNO(targetPort);

	// Send a midi packet to device
	Wire.beginTransmission(deviceId);
	Wire.write((uint8_t *)pk,sizeof(midiPacket_t));
	Wire.endTransmission();

}

///////////////////////////////////////////////////////////////////////////////
// THIS IS INSIDE AN ISR ! - PARSE DATA FROM MASTER TO SYNC ROUTING RULES
//////////////////////////////////////////////////////////////////////////////
int8_t I2C_ParseDataSync(uint8_t dataType,uint8_t arg1,uint8_t arg2)
{
  // midiRoutingRule
  if (dataType == B_DTYPE_MIDI_ROUTING_RULES_CABLE  || dataType == B_DTYPE_MIDI_ROUTING_RULES_SERIAL )
  {
    midiRoutingRule_t *mr ;
    if (Wire.available() != sizeof(midiRoutingRule_t)) return -1;

    if ( dataType == B_DTYPE_MIDI_ROUTING_RULES_CABLE )
    {
        if (arg1 >= USBCABLE_INTERFACE_MAX)  return -1;
        mr = &EEPROM_Params.midiRoutingRulesCable[arg1];
    }
    else
    {
        if (arg1 >= B_SERIAL_INTERFACE_MAX)  return -1;
        mr = &EEPROM_Params.midiRoutingRulesSerial[arg1];
    }
    midiRoutingRule_t r;
    Wire.readBytes((uint8_t *)&r,sizeof(midiRoutingRule_t));
    if (memcmp((void*)mr,(void*)&r,sizeof(midiRoutingRule_t)))
    {
      memcpy((void*)mr,(void*)&r,sizeof(midiRoutingRule_t));
      I2C_SlaveSyncDoUpdate = true;
    }
  }
  else
  // midiRoutingRulesIntelliThru
  if ( dataType == B_DTYPE_MIDI_ROUTING_RULES_INTELLITHRU )
  {
      if (Wire.available() != sizeof(midiRoutingRuleJack_t)) return -1;
      if (arg1 >= B_SERIAL_INTERFACE_MAX)  return -1;
      midiRoutingRuleJack_t *mr;
      midiRoutingRuleJack_t r;
      Wire.readBytes((uint8_t *)&r,sizeof(midiRoutingRuleJack_t));
      mr = &EEPROM_Params.midiRoutingRulesIntelliThru[arg1];
      if (memcmp((void*)mr,(void*)&r,sizeof(midiRoutingRuleJack_t)))
      {
        memcpy((void*)mr,(void*)&r,sizeof(midiRoutingRuleJack_t));
        I2C_SlaveSyncDoUpdate = true;
      }
  }
  else
  // intelliThruJackInMsk
  if (dataType == B_DTYPE_MIDI_ROUTING_INTELLITHRU_JACKIN_MSK) {
    if (Wire.available() != sizeof(uint16_t)) return -1;
    uint16_t jmsk;
    Wire.readBytes((uint8*)&jmsk,sizeof(uint16_t));
    if ( EEPROM_Params.intelliThruJackInMsk != jmsk) {
        EEPROM_Params.intelliThruJackInMsk = jmsk;
        I2C_SlaveSyncDoUpdate = true;
    }
  }
  else
  // intelliThruDelayPeriod
  if (dataType == B_DTYPE_MIDI_ROUTING_INTELLITHRU_DELAY_PERIOD) {
    if (Wire.available() != sizeof(uint8_t) ) return -1;
    uint8_t dp = Wire.read();
    if ( EEPROM_Params.intelliThruDelayPeriod != dp) {
      EEPROM_Params.intelliThruDelayPeriod = dp;
      I2C_SlaveSyncDoUpdate = true;
    }
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// THIS IS INSIDE AN ISR ! - PARSE IMMEDIATE Command
//////////////////////////////////////////////////////////////////////////////
//    Manage immediate commands not followed by a requestFrom
void I2C_ParseImmediateCmd() {

  I2C_Command = Wire.read();

  switch (I2C_Command)
  {
    case B_CMD_USBCX_UNAVAILABLE:
      midiUSBCx = false;
      break;

    case B_CMD_USBCX_AVAILABLE:
      midiUSBCx = true;
      break;

    case B_CMD_USBCX_SLEEP:
      midiUSBIdle = true;
      break;

    case B_CMD_USBCX_AWAKE:
      midiUSBIdle = false;
      break;

    case B_CMD_INTELLITHRU_ENABLED:
      intelliThruActive = true;
      break;

    case B_CMD_INTELLITHRU_DISABLED:
      intelliThruActive = false;
      break;

    case B_CMD_HARDWARE_RESET:
      nvic_sys_reset();
      break;

    case B_CMD_START_SYNC:
      I2C_SlaveSyncStarted = true;
      break;

    case B_CMD_END_SYNC:
      if ( I2C_SlaveSyncDoUpdate ) {
          // For now, the slave doesn't save master update as it is ALWAYS
          // synchronized at boot time.
          I2C_SlaveSyncDoUpdate = false;
      }
      I2C_SlaveSyncStarted = false;
      break;

  #ifdef DEBUG_MODE
    case B_CMD_DEBUG_MODE_ENABLED:
        // The master can enable degug mode.
        EEPROM_Params.debugMode = true;
      break;

    case B_CMD_DEBUG_MODE_DISABLED:
        // The master can disable degug mode.
        EEPROM_Params.debugMode = false;
        break;
  #endif
  }
}

///////////////////////////////////////////////////////////////////////////////
// THIS IS AN ISR ! - I2C Receive event trigger for a SLAVE
//////////////////////////////////////////////////////////////////////////////
void I2C_SlaveReceiveEvent(int howMany)
{

  I2C_MasterReadyTimeoutMillis = millis();

  // Update timer to avoid a permanent flash due to polling
  if (I2C_LedTimer.start() ) flashLED_CONNECT->start();

  // Is it a DATA SYNC ?
  if ( I2C_SlaveSyncStarted ) {

    if (howMany <= 0 ) {
      I2C_SlaveSyncStarted = false; I2C_SlaveSyncDoUpdate = false; // Abort
      return;
    }

    // Command. Save to EEPROM if the END sync is received
    if (howMany == 1 ) {
      I2C_ParseImmediateCmd();
      return;
    }

    // Error. No data.
    if (howMany <=3 ) {
      I2C_SlaveSyncStarted = false; I2C_SlaveSyncDoUpdate = false; // Abort
      return;
    }

    // Correct message format. Parse the data.
    uint8_t dataType = Wire.read();
    uint8_t arg1 = Wire.read();
    uint8_t arg2 = Wire.read();

    if  ( I2C_ParseDataSync(dataType,arg1,arg2) != 0) {
      I2C_SlaveSyncStarted = false; I2C_SlaveSyncDoUpdate = false; // Abort
      return;
    }
  }
  else
  {
    if (howMany <= 0) return;
    // It's a command ?
    // Immediate commands are managed here.
    // Those necessiting a requestFrom in the requestEvent ISR
    if (howMany == 1) {
      I2C_ParseImmediateCmd();
    }
    else // We only store packet here.
  	if (howMany == sizeof(midiPacket_t) ) {
  		midiPacket_t pk;
      // Read the bus and Write a packet in the ring buffer
      Wire.readBytes( (uint8_t*)&pk,sizeof(midiPacket_t) );
  		I2C_QPacketsFromMaster.write((uint8_t *)&pk,sizeof(midiPacket_t) );
  	}
  }
}

///////////////////////////////////////////////////////////////////////////////
// THIS IS AN ISR ! - I2C Request From Master event trigger. SLAVE ONLY..
//////////////////////////////////////////////////////////////////////////////
void I2C_SlaveRequestEvent ()
{
  if (I2C_LedTimer.start() ) flashLED_CONNECT->start();

  switch (I2C_Command) {

    case B_CMD_ISPACKET_AVAIL: {
        uint8_t nb = I2C_QPacketsToMaster.available() / sizeof(masterMidiPacket_t);
        Wire.write(nb);
        break;
    }

    case B_CMD_GET_MPACKET: {
        masterMidiPacket_t mpk;
        if (I2C_QPacketsToMaster.available()) {
          I2C_QPacketsToMaster.readBytes(mpk.packet,sizeof(masterMidiPacket_t));
        }
        else memset(mpk.packet,0,sizeof(masterMidiPacket_t));  // a null packet
        Wire.write(mpk.packet,sizeof(masterMidiPacket_t));
        break;
    }

    case B_CMD_IS_SLAVE_READY:
        Wire.write(B_STATE_READY);
        break;
  }

  I2C_Command = B_CMD_NONE;
}

///////////////////////////////////////////////////////////////////////////////
//  I2C Bus Checking.
//////////////////////////////////////////////////////////////////////////////
void I2C_BusChecks()
{
	if ( EEPROM_Params.I2C_DeviceId < B_SLAVE_DEVICE_BASE_ADDR &&
			 EEPROM_Params.I2C_DeviceId > B_SLAVE_DEVICE_LAST_ADDR )

			 EEPROM_Params.I2C_BusModeState = B_DISABLED; // Overwrite setting
}

///////////////////////////////////////////////////////////////////////////////
//  I2C Bus Start WIRE
//////////////////////////////////////////////////////////////////////////////
void I2C_BusStartWire()
{
	if ( EEPROM_Params.I2C_BusModeState == B_DISABLED ) return;

  // Attempt to reset the bus
  Wire.begin(); delay(10);  Wire.end();

	if ( EEPROM_Params.I2C_DeviceId == B_MASTERID ) {

      Wire.setClock(B_FREQ) ;
      // NB : default timemout is 1 sec. Possible to change that with Wire.setTimeout(x);
      // before the begin.
    	Wire.begin();

      // Reset all slaves if they are listening
      I2C_SendCommand(0,B_CMD_HARDWARE_RESET);
      delay(2000);

      // Scan BUS for active slave DEVICES. Table used only by the master.
      // 3 rounds to collect all devices.
      for ( uint8_t d=0; d < sizeof(I2C_DeviceActive) ; d++) I2C_DeviceActive[d] = false;

      uint8_t deviceId;
      uint8_t sCount = 0;

      for ( uint8_t i=1; i <= 3 && sCount <= sizeof(I2C_DeviceActive)  ; i ++ )  {
  			for ( uint8_t d=0; d < sizeof(I2C_DeviceActive) ; d++) {
          deviceId = d + B_SLAVE_DEVICE_BASE_ADDR;
          if ( I2C_SendCommand(deviceId,B_CMD_IS_SLAVE_READY) == B_STATE_READY ) {
            I2C_DeviceActive[d] = true;
            sCount++;

DEBUG_BEGIN
DEBUG_PRINTLN("Slave Id found :",deviceId);
DEBUG_END
          }
          delay(100);
  			}

       }

DEBUG_BEGIN
DEBUG_PRINTLN("SLAVES FOUND :",sCount);
DEBUG_END

       // If no slave found, reboot master in config mode
       if ( !sCount ) {

DEBUG_BEGIN
DEBUG_PRINTLN("NO SLAVES FOUND. Reboot in config mode","");
DEBUG_END

         EEPROM_Params.nextBootMode = bootModeConfigMenu;
         EEPROM_ParamsSave();
         nvic_sys_reset();
       }

       // Synchronize slaves routing rules
       I2C_SlavesRoutingSyncFromMaster();

	}
		// Slave initialization
	else 	{

      // NB : default timemout is 1 sec. Possible to change that with Wire.setTimeout(x);
      // before the begin.
      Wire.begin(EEPROM_Params.I2C_DeviceId);
	  	Wire.onRequest(I2C_SlaveRequestEvent);
			Wire.onReceive(I2C_SlaveReceiveEvent);

      // start USB serial
  		Serial.begin(115200);
      Serial.flush();
			delay(500);

      ShowMidiKliKHeader();Serial.println();

      Serial.println("Type 'C' for configuration menu (need to reboot).");
      Serial.println("Type 'R' to show current routing rules.");
			Serial.print("Slave ");Serial.print(EEPROM_Params.I2C_DeviceId);
			Serial.println(" ready and listening.");

      DEBUG_BEGIN
      DEBUG_PRINTLN("DEBUG MODE ACTIVE.","");
      DEBUG_END
	}
}

///////////////////////////////////////////////////////////////////////////////
// SEND an USBMIDIKLIK command on the I2C bus and wait for answer. MASTER ONLY
// If return is < 0 : error, else return the number of bytes received
//////////////////////////////////////////////////////////////////////////////
int16_t I2C_SendCommand(uint8_t deviceId,BusCommand cmd)
{
    Wire.beginTransmission(deviceId);
    Wire.write(cmd);

    if ( Wire.endTransmission() ) return -1;

    if ( BusCommandRequestSize[cmd] > 0) {
        uint8_t nb = Wire.requestFrom(deviceId,BusCommandRequestSize[cmd]);
        if ( nb != BusCommandRequestSize[cmd] ) return -1 ;
        if ( BusCommandRequestSize[cmd] == 1) return Wire.read();
        return nb;
    }

    return 0;
}
///////////////////////////////////////////////////////////////////////////////
//  I2C Show active slave device on screen
//////////////////////////////////////////////////////////////////////////////
void I2C_ShowActiveDevice()
{
	uint8_t deviceId;

	// Scan BUS for active slave DEVICES. Table used only by the master.
	for ( uint8_t d=0; d < sizeof(I2C_DeviceActive) ; d++) {
			deviceId = d + B_SLAVE_DEVICE_BASE_ADDR;
			Wire.beginTransmission(deviceId);
      delay(5);
			if ( Wire.endTransmission() == 0 ) {
					Serial.print("Slave device ")	;Serial.print(deviceId);
					Serial.println(" is active.");
			}
	}

}

///////////////////////////////////////////////////////////////////////////////
// Prepare a packet and route it to the right USB midi cable
///////////////////////////////////////////////////////////////////////////////
static void SerialMidi_RouteMsg( uint8_t cable, midiXparser* xpMidi )
{

    midiPacket_t pk = { .i = 0 };
    uint8_t msgLen = xpMidi->getMidiMsgLen();
    uint8_t msgType = xpMidi->getMidiMsgType();

    pk.packet[0] = cable << 4;
    memcpy(&pk.packet[1],&(xpMidi->getMidiMsg()[0]),msgLen);

    // Real time single byte message CIN F->
    if ( msgType == midiXparser::realTimeMsgTypeMsk ) pk.packet[0]   += 0xF;
    else

    // Channel voice message => CIN A-E
    if ( msgType == midiXparser::channelVoiceMsgTypeMsk )
        pk.packet[0]  += ( (xpMidi->getMidiMsg()[0]) >> 4);
    else

    // System common message CIN 2-3
    if ( msgType == midiXparser::systemCommonMsgTypeMsk ) {

        // 5 -  single-byte system common message (Tune request is the only case)
        if ( msgLen == 1 ) pk.packet[0] += 5;

        // 2/3 - two/three bytes system common message
        else pk.packet[0] += msgLen;
    }

    else return; // We should never be here !

    RoutePacketToTarget( FROM_SERIAL,&pk);
}

///////////////////////////////////////////////////////////////////////////////
// Parse sysex flows and make a packet for USB
// ----------------------------------------------------------------------------
// We use the midiXparser 'on the fly' mode, allowing to tag bytes as "captured"
// when they belong to a midi SYSEX message, without storing them in a buffer.
///////////////////////////////////////////////////////////////////////////////
static void SerialMidi_RouteSysEx( uint8_t cable, midiXparser* xpMidi )
{

  static midiPacket_t pk[SERIAL_INTERFACE_MAX];
  static uint8_t packetLen[SERIAL_INTERFACE_MAX];
  static bool firstCall = true;

  uint8_t readByte = xpMidi->getByte();

  // Initialize everything at the first call
  if (firstCall ) {
    firstCall = false;
    memset(pk,0,sizeof(midiPacket_t)*SERIAL_INTERFACE_MAX);
    memset(packetLen,0,sizeof(uint8_t)*SERIAL_INTERFACE_MAX);
  }

  // Normal End of SysEx or : End of SysEx with error.
  // Force clean end of SYSEX as the midi usb driver
  // will not understand if we send the packet as is
  if ( xpMidi->wasSysExMode() ) {
      // Force the eox byte in case we have a SYSEX error.
      packetLen[cable]++;
      pk[cable].packet[ packetLen[cable] ] = midiXparser::eoxStatus;
      // CIN = 5/6/7  sysex ends with one/two/three bytes,
      pk[cable].packet[0] = (cable << 4) + (packetLen[cable] + 4) ;
      RoutePacketToTarget( FROM_SERIAL,&pk[cable]);
      packetLen[cable] = 0;
      pk[cable].i = 0;
			return;
  } else

  // Fill USB sysex packet
  if ( xpMidi->isSysExMode() ) {
	  packetLen[cable]++;
	  pk[cable].packet[ packetLen[cable] ] = readByte ;

	  // Packet complete ?
	  if (packetLen[cable] == 3 ) {
	      pk[cable].packet[0] = (cable << 4) + 4 ; // Sysex start or continue
	      RoutePacketToTarget( FROM_SERIAL,&pk[cable]);
	      packetLen[cable] = 0;
	      pk[cable].i = 0;
	  }
	}
}

///////////////////////////////////////////////////////////////////////////////
// PARSE INTERNAL SYSEX, either for serial or USB
// ----------------------------------------------------------------------------
// Internal sysex must be transmitted on the first cable/midi jack (whatever routing).
// Internal sysex are purely ignored on other cables or jack.
///////////////////////////////////////////////////////////////////////////////
static void ParseSysExInternal(const midiPacket_t *pk)
{

		static unsigned sysExInternalMsgIdx = 0;
		static bool 	sysExInternalHeaderFound = false;

		uint8_t cin   = pk->packet[0] & 0x0F ;
    uint8_t cable = pk->packet[0] >> 4;

		// Only SYSEX and concerned packet on cable or serial 1
    if (cable > 0) return;
		if (cin < 4 || cin > 7) return;
		if (cin == 4 && pk->packet[1] != 0xF0 && sysExInternalMsgIdx<3 ) return;
		if (cin > 4  && sysExInternalMsgIdx<3) return;

		uint8_t pklen = ( cin == 4 ? 3 : cin - 4) ;
		uint8_t ev = 1;

		for ( uint8_t i = 0 ; i< pklen ; i++ ) {
			if (sysExInternalHeaderFound) {
				// Start storing the message in the msg buffer
				// If Message too big. don't store...
				if ( sysExInternalBuffer[0] <  sizeof(sysExInternalBuffer)-1  ) {
						if (pk->packet[ev] != 0xF7) {
							sysExInternalBuffer[0]++;
							sysExInternalBuffer[sysExInternalBuffer[0]]  = pk->packet[ev];
						}
						ev++;
				}
			}	else

			if ( sysExInternalHeader[sysExInternalMsgIdx] == pk->packet[ev] ) {
				sysExInternalMsgIdx++;
				ev++;
				if ( sysExInternalMsgIdx >= sizeof(sysExInternalHeader) ) {
					sysExInternalHeaderFound = true;
					sysExInternalBuffer[0] = 0; // Len of the sysex buffer
				}
			}

			else {
				// No match
				sysExInternalMsgIdx = 0;
				sysExInternalHeaderFound = false;
				return;
			}
		}

		// End of SYSEX for a valid message ? => Process
		if (cin != 4  && sysExInternalHeaderFound ) {
			sysExInternalMsgIdx = 0;
			sysExInternalHeaderFound = false;
			ProcessSysExInternal();
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
	if ( source == FROM_SERIAL ) {
    if ( port >= SERIAL_INTERFACE_MAX ) return;
    // If bus mode active, the local port# must be translated according
		// to the device Id, before routing
    if (EEPROM_Params.I2C_BusModeState == B_ENABLED ) {
			port = GET_BUS_SERIALNO_FROM_LOCALDEV(EEPROM_Params.I2C_DeviceId,port);
    }
  }

  uint8_t cin   = pk->packet[0] & 0x0F ;

	FLASH_LED_IN(thisLed);

	// Sysex is a particular case when using packets.
	// Internal sysex Jack 1/Cable 0 ALWAYS!! are checked whatever filters are
	// This insures that the internal sysex will be always interpreted.
	// If the MCU is resetted, the msg will not be sent
	uint8_t  msgType=0;

	if (cin >= 4 && cin <= 7  ) {
		if (port == 0) ParseSysExInternal(pk);
		msgType =  midiXparser::sysExMsgTypeMsk;
	} else {
			msgType =  midiXparser::getMidiStatusMsgTypeMsk(pk->packet[1]);
	}

	// ROUTING tables
	uint16_t *cableInTargets ;
	uint16_t *serialOutTargets ;
	uint8_t *inFilters ;

  if (source == FROM_SERIAL ){
    // IntelliThru active ? If so, take the good routing rules
    if ( intelliThruActive ) {
			if ( ! EEPROM_Params.intelliThruJackInMsk ) return; // Double check.
      serialOutTargets = &EEPROM_Params.midiRoutingRulesIntelliThru[port].jackOutTargetsMsk;
      inFilters = &EEPROM_Params.midiRoutingRulesIntelliThru[port].filterMsk;
    }
    else {
      cableInTargets = &EEPROM_Params.midiRoutingRulesSerial[port].cableInTargetsMsk;
      serialOutTargets = &EEPROM_Params.midiRoutingRulesSerial[port].jackOutTargetsMsk;
      inFilters = &EEPROM_Params.midiRoutingRulesSerial[port].filterMsk;
    }
  }
  else if (source == FROM_USB ) {
      cableInTargets = &EEPROM_Params.midiRoutingRulesCable[port].cableInTargetsMsk;
      serialOutTargets = &EEPROM_Params.midiRoutingRulesCable[port].jackOutTargetsMsk;
      inFilters = &EEPROM_Params.midiRoutingRulesCable[port].filterMsk;
  }

  else return; // Error.


	// Apply midi filters
	if (! (msgType & *inFilters) ) return;

	// ROUTING FROM ANY SOURCE PORT TO SERIAL TARGETS //////////////////////////
	// A target match ?
  if ( *serialOutTargets) {
				for (	uint16_t t=0; t<SERIAL_INTERFACE_COUNT ; t++)
					if ( (*serialOutTargets & ( 1 << t ) ) ) {
								// Route via the bus
								if (EEPROM_Params.I2C_BusModeState == B_ENABLED ) {
                     I2C_BusSerialSendMidiPacket(pk, t);
								}
								// Route to local serial if bus mode disabled
								else SerialMidi_SendPacket(pk,t);
					}
	} // serialOutTargets

  // Stop here if IntelliThru active (no USB active but maybe connected)
  // Intellithru is always activated by the master in bus mode.

  if ( intelliThruActive ) return;

  // Stop here if no USB connection (owned by the master).
  // If we are a slave, the master should have notified us
  if ( ! midiUSBCx ) return;

	// Apply cable routing rules from serial or USB
	// Only if USB connected and thru mode inactive

  if (  *cableInTargets  ) {
    	midiPacket_t pk2 = { .i = pk->i }; // packet copy to change the dest cable
			for (uint8_t t=0; t < USBCABLE_INTERFACE_MAX ; t++) {
	      if ( *cableInTargets & ( 1 << t ) ) {
	          pk2.packet[0] = ( t << 4 ) + cin;

            // Only the master has USB midi privilege in bus MODE
            // Everybody else if an usb connection is active
            if (! B_IS_SLAVE ) {
                MidiUSB.writePacket(&(pk2.i));
DEBUG_BEGIN
DEBUG_PRINTLN("PK to local master USB","");
DEBUG_END

            } else
            // A slave in bus mode ?
            // We need to add a master packet to the Master's queue.
            {
                masterMidiPacket_t mpk;
                mpk.mpk.dest = TO_USB;
                // Copy the midi packet to the master packet
                mpk.mpk.pk.i = pk2.i;

                I2C_QPacketsToMaster.write(mpk.packet,sizeof(masterMidiPacket_t));
DEBUG_BEGIN
DEBUG_PRINTLN("PK dest USB    to master Q.count=",I2C_QPacketsToMaster.available());
DEBUG_END
            }

	          #ifdef LEDS_MIDI
	          flashLED_IN[t]->start();
	          #endif
	    	}
			}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Reset routing rules to default factory
// ROUTING_RESET_ALL         : Factory defaults
// ROUTING_RESET_MIDIUSB     : Midi USB and serial routing to defaults
// ROUTING_RESET_INTELLITHRU : Intellithru to factory defaults
// ROUTING_INTELLITHRU_OFF   : Stop IntelliThru
///////////////////////////////////////////////////////////////////////////////
void ResetMidiRoutingRules(uint8_t mode)
{

	if (mode == ROUTING_RESET_ALL || mode == ROUTING_RESET_MIDIUSB) {

	  for ( uint8_t i = 0 ; i < USBCABLE_INTERFACE_MAX ; i++ ) {

			// Cables
	    EEPROM_Params.midiRoutingRulesCable[i].filterMsk = midiXparser::allMsgTypeMsk;
	    EEPROM_Params.midiRoutingRulesCable[i].cableInTargetsMsk = 0 ;
	    EEPROM_Params.midiRoutingRulesCable[i].jackOutTargetsMsk = 1 << i ;
		}

		for ( uint8_t i = 0 ; i < B_SERIAL_INTERFACE_MAX ; i++ ) {

			// Jack serial
	    EEPROM_Params.midiRoutingRulesSerial[i].filterMsk = midiXparser::allMsgTypeMsk;
	    EEPROM_Params.midiRoutingRulesSerial[i].cableInTargetsMsk = 1 << i ;
	    EEPROM_Params.midiRoutingRulesSerial[i].jackOutTargetsMsk = 0  ;
	  }
	}

	if (mode == ROUTING_RESET_ALL || mode == ROUTING_RESET_INTELLITHRU) {
	  // "Intelligent thru" serial mode
	  for ( uint8_t i = 0 ; i < B_SERIAL_INTERFACE_MAX ; i++ ) {
	    EEPROM_Params.midiRoutingRulesIntelliThru[i].filterMsk = midiXparser::allMsgTypeMsk;
	    EEPROM_Params.midiRoutingRulesIntelliThru[i].jackOutTargetsMsk = 0B1111 ;
		}
		EEPROM_Params.intelliThruJackInMsk = 0;
	  EEPROM_Params.intelliThruDelayPeriod = DEFAULT_INTELLIGENT_MIDI_THRU_DELAY_PERIOD ;
	}

	// Disable "Intelligent thru" serial mode
	if (mode == ROUTING_INTELLITHRU_OFF ) {
		for ( uint8_t i = 0 ; i < B_SERIAL_INTERFACE_MAX ; i++ ) {
			EEPROM_Params.intelliThruJackInMsk = 0;
		}
	}

}

///////////////////////////////////////////////////////////////////////////////
// Process internal USBMidiKlik SYSEX
// ----------------------------------------------------------------------------
// MidiKlik SYSEX are of the following form :
//
// F0            SOX Start Of Sysex
// 77 77 78      USBMIDIKliK header
// <xx>          USBMIDIKliK sysex command
// <dddddd...dd> data
// F7            EOX End of SYSEX
//
// SOX, Header and EOX are not stored in sysExInternalBuffer.
//
// Cmd Description                           Data
//
// 0x08 Reboot in config mode
// 0x0A Hard reset interface
// 0x0B Change USB Product string
// 0X0C Change USB VID / PID
// 0X0E Set Intelligent Midi thru mode
// 0X0F Change input routing rule
// ----------------------------------------------------------------------------
// sysExInternalBuffer[0] length of the message (func code + data)
// sysExInternalBuffer[1] function code
// sysExInternalBuffer[2] data without EOX
///////////////////////////////////////////////////////////////////////////////
static void ProcessSysExInternal()
{

  uint8_t msgLen = sysExInternalBuffer[0];
  uint8_t cmdId  = sysExInternalBuffer[1];

  switch (cmdId) {

    // TEMP REBOOT IN CONFIG MODE
		// F0 77 77 78 08 F7
		case 0x08:
			// Set serial boot mode & Write the whole param struct
			EEPROM_Params.nextBootMode = bootModeConfigMenu;
      EEPROM_ParamsSave();
      nvic_sys_reset();
			break;

    // RESET USB MIDI INTERFACE -----------------------------------------------
    // F0 77 77 78 0A F7
    case 0x0A:
      nvic_sys_reset();
      break;

    // CHANGE MIDI PRODUCT STRING ---------------------------------------------
    // F0 77 77 78 0B <character array> F7
    case 0x0B:
      // Copy the receive message to the Product String Descriptor
      // For MIDI protocol compatibility, and avoid a sysex encoding,
      // Accentuated ASCII characters, below 128 non supported.

      if (msgLen < 2) break;

      if ( (msgLen-1) > USB_MIDI_PRODUCT_STRING_SIZE  ) {
          // Error : Product name too long
          break;
      }

      // Store the new sting in EEPROM bloc
      memset(&EEPROM_Params.productString,0, sizeof(EEPROM_Params.productString));
      memcpy(&EEPROM_Params.productString,&sysExInternalBuffer[2],msgLen-1);

      // Write the whole param struct
      EEPROM_ParamsSave();

      break;

    // VENDOR ID & PRODUCT ID -------------------------------------------------
    // F0 77 77 78 0C <n1 n2 n3 n4 = Vendor Id nibbles> <n1 n2 n3 n4 = Product Id nibbles> F7
    case 0x0C:
      // As MIDI data are 7 bits bytes, we must use a special encoding, to encode 8 bits values,
      // as light as possible. As we have here only 2 x 16 bits values to handle,
      // the encoding will consists in sending each nibble (4 bits) serialized in bytes.
      // For example sending VendorID and ProductID 0X8F12 0X9067 will be encoded as :
      //   0x08 0XF 0x1 0x2  0X9 0X0 0X6 0X7,  so the complete SYSEX message will be :
      // F0 77 77 78 0C 08 0F 01 02 09 00 06 07 F7

      if ( msgLen != 9 ) break;
      EEPROM_Params.vendorID = (sysExInternalBuffer[2] << 12) + (sysExInternalBuffer[3] << 8) +
                                     (sysExInternalBuffer[4] << 4) + sysExInternalBuffer[5] ;
      EEPROM_Params.productID= (sysExInternalBuffer[6] << 12) + (sysExInternalBuffer[7] << 8) +
                                     (sysExInternalBuffer[8] << 4) + sysExInternalBuffer[9] ;
      EEPROM_ParamsSave();

      break;

		// Intelligent MIDI THRU. -------------------------------------------------
		// When USB midi is not active beyond a defined timout, the MIDI THRU mode can be activated.
		// After that timout, every events from the MIDI INPUT Jack #n will be routed to outputs jacks 1-4,
		// accordingly with the midi thru mode serial jack targets mask.
		//
    // Header       = F0 77 77 78
		// Function     = 0E
		//
		// Action       =
		//  00 Reset to default
		//  01 Disable
		//  02 Set Delay <number of 15s periods 1-127>
		//  03 Set thu mode jack routing +
		//          . Midi In Jack = < Midi In Jack # = 0-F>
		//          . Midi Msg filter mask
		//                  zero if you want to inactivate intelliThru for this jack
		//                  channel Voice = 0001 (1),
		//                  system Common = 0010 (2),
		//                  realTime      = 0100 (4),
		//                  sysEx         = 1000 (8)
		//          . Serial midi Jack out targets 1 < Midi Out Jack # 1-n = 0-n>
		//          . Serial midi Jack out targets 2
		//          . Serial midi Jack out targets 3
		//                     ......
		//          . Serial midi Jack out targets n <= 16
		//
		// EOX = F7
		//
		// Examples :
		// F0 77 77 78 0E 00 F7    <= Reset to default
		// F0 77 77 78 0E 01 F7    <= Disable
		// F0 77 77 78 0E 02 02 F7 <= Set delay to 30s
		// F0 77 77 78 0E 03 01 0F 00 01 02 03 F7<= Set Midi In Jack 2 to Jacks out 1,2,3,4 All msg
		// F0 77 77 78 0E 03 03 0C 03 04 F7 <= Set Midi In Jack 4 to Jack 3,4, real time only

    case 0x0E:

			if ( msgLen < 2 ) break;

			// reset to default midi thru routing
      if (sysExInternalBuffer[2] == 0x00  && msgLen == 2) {
				 ResetMidiRoutingRules(ROUTING_RESET_INTELLITHRU);
			} else

			// Disable thru mode
			if (sysExInternalBuffer[2] == 0x01  && msgLen == 3) {
					ResetMidiRoutingRules(ROUTING_INTELLITHRU_OFF);
			}

			else
			// Set Delay
			// The min delay is 1 period of 15 secondes. The max is 127 periods of 15 secondes.
      if (sysExInternalBuffer[2] == 0x02  && msgLen == 3) {
				if ( sysExInternalBuffer[3] < 1 || sysExInternalBuffer[3] > 0X7F ) break;
				EEPROM_Params.intelliThruDelayPeriod = sysExInternalBuffer[3];
			}

			else
			// Set routing : Midin 2 mapped to all events. All 4 ports.
			// 0E 03 01 0F 00 01 02 03
			// 0E 03 01 0F 00

			if (sysExInternalBuffer[2] == 3 ) {

					if (msgLen < 4 ) break;
					if ( msgLen > SERIAL_INTERFACE_COUNT + 4 ) break;

					uint8_t src = sysExInternalBuffer[3];
					uint8_t filterMsk = sysExInternalBuffer[4];

					if ( src >= SERIAL_INTERFACE_COUNT) break;

					// Filter is 4 bits
					if ( filterMsk > 0x0F  ) break;

					// Disable Thru mode for this jack if no filter
					if ( filterMsk == 0 && msgLen ==4 ) {
						EEPROM_Params.intelliThruJackInMsk &= ~(1 << src);
					}
					else {
							if ( filterMsk == 0 ) break;
							// Add midi in jack
							EEPROM_Params.intelliThruJackInMsk |= (1 << src);
							// Set filter
							EEPROM_Params.midiRoutingRulesIntelliThru[src].filterMsk = filterMsk;
							// Set Jacks
							if ( msgLen > 4)	{
								uint16_t msk = 0;
								for ( uint8_t i = 5 ; i< (msgLen+1)  ; i++) {
										if ( sysExInternalBuffer[i] < SERIAL_INTERFACE_COUNT)
											msk |= 	1 << sysExInternalBuffer[i] ;
								}
								EEPROM_Params.midiRoutingRulesIntelliThru[src].jackOutTargetsMsk = msk;
							}
					}
			}
			else	break;

			// Write the whole param struct
      EEPROM_ParamsSave();

			// reset globals for a real time update
			intelliThruDelayMillis = EEPROM_Params.intelliThruDelayPeriod * 15000;

      // Synchronize slaves routing rules
      if (B_IS_MASTER) I2C_SlavesRoutingSyncFromMaster();

      break;

    // SET ROUTING TARGETS ----------------------------------------------------
		// Header       = F0 77 77 78
		// Function     = 0F
		// Action       =
		//  00 Reset to default midi routing
		//  01 Set routing +
    //          . source type     = <cable=0X0 | serial=0x1>
		//          . id              = id for cable or serial 0-F
		//          . destination type = <cable=0X0 | serial=0x1>
		//          . routing targets = <cable 0 1 2 n> or <jack 0 1 2 n> - 0-F
		//  02 Set filter msk :
    //          . source type     = <cable=0X0 | serial=0x1>
		//          . id              = id for cable or serial 0-F
		//          . midi filter mask
		// EOX = F7
		//
		// Midi filter mask is defined as a midiXparser message type mask.
		// noneMsgTypeMsk          = 0B0000,
		// channelVoiceMsgTypeMsk  = 0B0001,
		// systemCommonMsgTypeMsk  = 0B0010,
		// realTimeMsgTypeMsk      = 0B0100,
		// sysExMsgTypeMsk         = 0B1000,
		// allMsgTypeMsk           = 0B1111
		//
    // Examples :
		// F0 77 77 78 0F 00 F7          <= reset to default midi routing
		// F0 77 77 78 0F 02 00 00 04 F7 <= Set filter to realtime events on cable 0
    // F0 77 77 78 0F 01 00 00 01 00 01 F7 <= Set Cable 0 to Jack 1,2
		// F0 77 77 78 0F 01 00 00 00 00 01 F7 <= Set Cable 0 to Cable In 0, In 01
		// F0 77 77 78 0F 01 00 00 01 00 01 F7 <= & jack 1,2 (2 msg)
		// F0 77 77 78 0F 01 01 01 01 00 01 02 03 F7 <= Set Serial jack In No 2 to 4 serial jack out

    case 0x0F:

      // reset to default routing

      if (sysExInternalBuffer[2] == 0x00  && msgLen == 2) {
					ResetMidiRoutingRules(ROUTING_RESET_MIDIUSB);
      } else

			// Set filter

			if (sysExInternalBuffer[2] == 0x02  && msgLen == 5) {

					uint8_t srcType = sysExInternalBuffer[3];
					uint8_t src = sysExInternalBuffer[4];
					uint8_t filterMsk = sysExInternalBuffer[5];

					// Filter is 4 bits
				  if ( filterMsk > 0x0F  ) break;

					if (srcType == 0 ) { // Cable
						if ( src  >= USBCABLE_INTERFACE_MAX) break;
								EEPROM_Params.midiRoutingRulesCable[src].filterMsk = filterMsk;
					} else

					if (srcType == 1) { // Serial
						if ( src >= SERIAL_INTERFACE_COUNT) break;
							EEPROM_Params.midiRoutingRulesSerial[src].filterMsk = filterMsk;
					} else break;


			} else

			// Set Routing targets

			if (sysExInternalBuffer[2] == 0x01  )
      {

				if (msgLen < 6) break;

				uint8_t srcType = sysExInternalBuffer[3];
				uint8_t dstType = sysExInternalBuffer[5];
				uint8_t src = sysExInternalBuffer[4];

				if (srcType != 0 && srcType != 1 ) break;
				if (dstType != 0 && dstType != 1 ) break;
				if (srcType  == 0 && src >= USBCABLE_INTERFACE_MAX ) break;
				if (srcType  == 1 && src >= SERIAL_INTERFACE_COUNT) break;
				if (dstType  == 0 &&  msgLen > (USBCABLE_INTERFACE_MAX + 5) )  break;
				if (dstType  == 1 &&  msgLen > (SERIAL_INTERFACE_COUNT + 5) )  break;

				// Compute mask from the port list
				uint16_t msk = 0;
				for ( uint8_t i = 6 ; i< (msgLen+1)  ; i++) {
					  uint8_t b = sysExInternalBuffer[i];
						if ( dstType == 0 && b < USBCABLE_INTERFACE_MAX ||
						     dstType == 1 && b < SERIAL_INTERFACE_COUNT) {

									 msk |= 	1 << b ;
						}
				}// for

				// Set masks
				if ( srcType == 0 ) { // Cable
						if (dstType == 0) // To cable
							EEPROM_Params.midiRoutingRulesCable[src].cableInTargetsMsk = msk;
						else // To serial
							EEPROM_Params.midiRoutingRulesCable[src].jackOutTargetsMsk = msk;

				} else

				if ( srcType == 1 ) { // Serial
					if (dstType == 0)
						EEPROM_Params.midiRoutingRulesSerial[src].cableInTargetsMsk = msk;
					else
						EEPROM_Params.midiRoutingRulesSerial[src].jackOutTargetsMsk = msk;
				}

      } else
					return;

			// Write the whole param struct
			EEPROM_ParamsSave();

      // Synchronize slaves routing rules
      if (B_IS_MASTER) I2C_SlavesRoutingSyncFromMaster();

			break;

  }

}

///////////////////////////////////////////////////////////////////////////////
// Intialize parameters stores in EEPROM
//----------------------------------------------------------------------------
// Retrieve global parameters from EEPROM, or Initalize it
// If factorySetting is true, all settings will be forced to factory default
//////////////////////////////////////////////////////////////////////////////
void EEPROM_ParamsInit(bool factorySettings=false)
{

  // Read the EEPROM parameters structure
  EEPROM_ParamsLoad();

  // If the signature is not found, of not the same version of parameters structure,
  // or new version, or new size then initialize (factory settings)

	// New fimware  uploaded
	if ( memcmp(EEPROM_Params.TimestampedVersion,TimestampedVersion,sizeof(EEPROM_Params.TimestampedVersion)) )
	{
		// Update timestamp and activate

		if  ( memcmp( EEPROM_Params.signature,EE_SIGNATURE,sizeof(EEPROM_Params.signature) )
						|| EEPROM_Params.prmVersion != EE_PRMVER
						|| ( EEPROM_Params.majorVersion != VERSION_MAJOR && EEPROM_Params.minorVersion != VERSION_MINOR )
						|| EEPROM_Params.size != sizeof(EEPROM_Params_t)
				) factorySettings = true;
		else
		// New build only. We keep existing settings but reboot in config mode
 		{
			memcpy( EEPROM_Params.TimestampedVersion,TimestampedVersion,sizeof(EEPROM_Params.TimestampedVersion) );
			EEPROM_Params.nextBootMode = bootModeMidi;
			EEPROM_ParamsSave();
			// Default boot mode when new firmware uploaded (not saved)
	    EEPROM_Params.nextBootMode = bootModeConfigMenu;
			return;

		}

	}


	// Force factory setting
  if (  factorySettings )
	{
    memset( &EEPROM_Params,0,sizeof(EEPROM_Params_t) );
    memcpy( EEPROM_Params.signature,EE_SIGNATURE,sizeof(EEPROM_Params.signature) );

		EEPROM_Params.majorVersion = VERSION_MAJOR;
		EEPROM_Params.minorVersion = VERSION_MINOR;

		EEPROM_Params.prmVersion = EE_PRMVER;
		EEPROM_Params.size = sizeof(EEPROM_Params_t);

    memcpy( EEPROM_Params.TimestampedVersion,TimestampedVersion,sizeof(EEPROM_Params.TimestampedVersion) );

    EEPROM_Params.debugMode = false;

		// Default I2C Device ID and bus mode
		EEPROM_Params.I2C_DeviceId = B_MASTERID;
		EEPROM_Params.I2C_BusModeState = B_DISABLED;

		ResetMidiRoutingRules(ROUTING_RESET_ALL);

    EEPROM_Params.vendorID  = USB_MIDI_VENDORID;
    EEPROM_Params.productID = USB_MIDI_PRODUCTID;

    memcpy(EEPROM_Params.productString,USB_MIDI_PRODUCT_STRING,sizeof(USB_MIDI_PRODUCT_STRING));

		EEPROM_Params.nextBootMode = bootModeMidi;

		//Write the whole param struct
    EEPROM_ParamsSave();

		// Default boot mode when new firmware uploaded (not saved as one shot mode)
		EEPROM_Params.nextBootMode = bootModeConfigMenu;

  }

}

///////////////////////////////////////////////////////////////////////////////
// Serial print a formated hex value
//////////////////////////////////////////////////////////////////////////////
void PrintCleanHEX(uint8_t hexVal)
{
          if ( hexVal < 0x10 ) Serial.print("0");
          Serial.print(hexVal,HEX);
}

///////////////////////////////////////////////////////////////////////////////
// DUMP a byte buffer to screen
//////////////////////////////////////////////////////////////////////////////
void ShowBufferHexDump(uint8_t* bloc, uint16_t sz)
{
	uint8_t j=0;
	uint8_t * pp = bloc;
	for (uint16_t i=0; i<sz; i++) {
				PrintCleanHEX(*pp);
				Serial.print(" ");
				if (++j > 16 ) { Serial.println(); j=0; }
				pp++;
	}
	//Serial.println();
}

void ShowBufferHexDumpDebugSerial(uint8_t* bloc, uint16_t sz)
{
	uint8_t j=0;
	uint8_t * pp = bloc;
	for (uint16_t i=0; i<sz; i++) {
        if ( *pp < 0x10 ) DEBUG_SERIAL.print("0");
        DEBUG_SERIAL.print(*pp,HEX);
				DEBUG_SERIAL.print(" ");
				if (++j > 16 ) { DEBUG_SERIAL.println(); j=0; }
				pp++;
	}
	//DEBUG_SERIAL.println();
}

///////////////////////////////////////////////////////////////////////////////
// Optimized put byte buffer to EEPROM
//////////////////////////////////////////////////////////////////////////////
void EEPROM_Put(uint8_t* bloc,uint16_t sz)
{
	uint16_t addressWrite = 0x10;
	uint8_t * pp = bloc;
	uint16_t value = 0;

	// Write 2 uint8_t in an uint16t
	for (uint16_t i=1; i<=sz; i++) {
			if ( i % 2 ) {
				value = *pp;
				value = value << 8;
			} else {
				value |= *pp;
				EEPROM.write(addressWrite++, value);
			}
			pp++;
	}
	// Proceed the last write eventually
	if (sz % 2 ) EEPROM.write(addressWrite, value);
}

///////////////////////////////////////////////////////////////////////////////
// Optimized get byte buffer to EEPROM
//////////////////////////////////////////////////////////////////////////////
void EEPROM_Get(uint8_t* bloc,uint16_t sz)
{
	uint16_t addressRead = 0x10;
	uint8_t * pp = bloc;
	uint16_t value = 0;

	// Read 2 uint8_t in an uint16t
	for (uint16_t i=1; i<=sz; i++) {
			if ( i % 2 ) {
				EEPROM.read(addressRead++, &value);
				*pp = value >> 8;
			} else {
				*pp = value & 0x00FF;
			}
			pp++;
	}

}

///////////////////////////////////////////////////////////////////////////////
// HIGH LEVEL LOAD PARAMETERS FROM EEPROM
//----------------------------------------------------------------------------
// High level abstraction parameters read function
//////////////////////////////////////////////////////////////////////////////
void EEPROM_ParamsLoad()
{

	EEPROM_Get((uint8*)&EEPROM_Params,sizeof(EEPROM_Params_t));

}

///////////////////////////////////////////////////////////////////////////////
// HIGH LEVEL SAVE PARAMETERS TO EEPROM
//----------------------------------------------------------------------------
// High level abstraction parameters read function
//////////////////////////////////////////////////////////////////////////////
void EEPROM_ParamsSave()
{

	EEPROM_Put((uint8*)&EEPROM_Params,sizeof(EEPROM_Params_t)) ;

}

///////////////////////////////////////////////////////////////////////////////
// Get an Int8 From a hex char.  letters are minus.
// Warning : No control of the size or hex validity!!
///////////////////////////////////////////////////////////////////////////////
static uint8_t GetInt8FromHexChar(char c)
{
	return (uint8_t) ( c <= '9' ? c - '0' : c - 'a' + 0xa );
}

///////////////////////////////////////////////////////////////////////////////
// Get an Int16 From a 4 Char hex array.  letters are minus.
// Warning : No control of the size or hex validity!!
///////////////////////////////////////////////////////////////////////////////
static uint16_t GetInt16FromHex4Char(char * buff)
{
	char val[4];

  for ( uint8_t i =0; i < sizeof(val) ; i++ )
    val[i] = GetInt8FromHexChar(buff[i]);

	return GetInt16FromHex4Bin(val);
}

///////////////////////////////////////////////////////////////////////////////
// Get an Int16 From a 4 hex digit binary array.
// Warning : No control of the size or hex validity!!
///////////////////////////////////////////////////////////////////////////////
static uint16_t GetInt16FromHex4Bin(char * buff)
{
	return (uint16_t) ( (buff[0] << 12) + (buff[1] << 8) + (buff[2] << 4) + buff[3] );
}

///////////////////////////////////////////////////////////////////////////////
// USB serial get a number of N digit (long)
///////////////////////////////////////////////////////////////////////////////
static uint16_t AsknNumber(uint8_t n)
{
	uint16_t v=0;
	uint8_t choice;
	while (n--) {
		v += ( (choice = AskDigit() ) - '0' )*pow(10,n);
		Serial.print(choice - '0');
	}
	return v;
}

///////////////////////////////////////////////////////////////////////////////
// USB serial getdigit
///////////////////////////////////////////////////////////////////////////////
static char AskDigit()
{
  char c;
  while ( ( c = AskChar() ) <'0' && c > '9');
  return c;
}

///////////////////////////////////////////////////////////////////////////////
// USB serial getchar
///////////////////////////////////////////////////////////////////////////////
static char AskChar()
{
  while (!Serial.available() >0);
  char c = Serial.read();
  // Flush
  while (Serial.available()>0) Serial.read();
  return c;
}

///////////////////////////////////////////////////////////////////////////////
// "Scanf like" for hexadecimal inputs
///////////////////////////////////////////////////////////////////////////////
static uint8_t AsknHexChar(char *buff, uint8_t len,char exitchar,char sepa)
{

	uint8_t i = 0, c = 0;

	while ( i < len ) {
		c = AskChar();
		if ( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'  ) ) {
			Serial.write(c);
			if (sepa) Serial.write(sepa);
			buff[i++]	= GetInt8FromHexChar(c);
		} else if (c == exitchar && exitchar !=0 ) break;
	}
	return i;
}

///////////////////////////////////////////////////////////////////////////////
// Get a choice from a question
///////////////////////////////////////////////////////////////////////////////
char AskChoice(const char * qLabel, char * choices)
{
	char c;
	char * yn = "yn";
	char * ch;

	if ( *choices == 0 ) ch = yn; else ch = choices;
	Serial.print(qLabel);
	Serial.print(" (");
	Serial.print(ch);
	Serial.print(") ? ");

	while (1) {
		c = AskChar();
		uint8_t i=0;
		while (ch[i] )
				if ( c == ch[i++] ) { Serial.write(c); return c; }
	}
}

///////////////////////////////////////////////////////////////////////////////
// Show a routing table line
///////////////////////////////////////////////////////////////////////////////
void ShowMidiRoutingLine(uint8_t port,uint8_t ruleType, void *anyRule)
{

	uint8_t  maxPorts = 0;
	uint8_t  filterMsk = 0;
	uint16_t cableInTargetsMsk = 0;
	uint16_t jackOutTargetsMsk = 0 ;

	if (ruleType == USBCABLE_RULE || ruleType == SERIAL_RULE ) {
			maxPorts = ( ruleType == USBCABLE_RULE ? USBCABLE_INTERFACE_MAX:SERIAL_INTERFACE_COUNT );
			filterMsk = ((midiRoutingRule_t *)anyRule)->filterMsk;
			cableInTargetsMsk = ((midiRoutingRule_t *)anyRule)->cableInTargetsMsk;
			jackOutTargetsMsk = ((midiRoutingRule_t *)anyRule)->jackOutTargetsMsk;
	}	else if (ruleType == INTELLITHRU_RULE ) {
			maxPorts = SERIAL_INTERFACE_COUNT;
			filterMsk = ((midiRoutingRuleJack_t *)anyRule)->filterMsk;
			jackOutTargetsMsk = ((midiRoutingRuleJack_t *)anyRule)->jackOutTargetsMsk;
	}
  else return;


	// No display if port is not exsiting or not concerned by IntelliThru
	if ( port >= maxPorts ) return;
	if (ruleType == INTELLITHRU_RULE && !(EEPROM_Params.intelliThruJackInMsk & (1<< port)) ) return;

	// print a full new line
	Serial.print("|");
	Serial.print(port+1);
	Serial.print( port+1 > 9 ? "":" ");
	Serial.print("|  ");

	// Filters
	for ( uint8_t f=0; f < 4 ; f++) {
		 if ( ( filterMsk & ( 1 << f) )  )
			 Serial.print("X  ");
		 else Serial.print(".  ");
	}

	// Cable In (but Midi Thru mode)
	Serial.print("| ");
	if (ruleType != INTELLITHRU_RULE) {
		for ( uint8_t cin=0; cin < 16 ; cin++) {
					if (cin >= USBCABLE_INTERFACE_MAX ) {
						Serial.print(" ");
					} else {
							if ( cableInTargetsMsk & ( 1 << cin) )
									Serial.print("X");
							else Serial.print(".");
					}
		}
	}	else Serial.print("     (N/A)      "); // IntelliThru

	// Jack Out
	Serial.print(" | ");
	for ( uint8_t jk=0; jk < 16 ; jk++) {
					if (jk >= SERIAL_INTERFACE_COUNT ) {
						Serial.print(" ");
					} else
					if ( jackOutTargetsMsk & ( 1 << jk) )
									Serial.print("X");
				  else Serial.print(".");
	}
	Serial.println(" |");

}

///////////////////////////////////////////////////////////////////////////////
// Show the midi routing map
///////////////////////////////////////////////////////////////////////////////
void ShowMidiRouting(uint8_t ruleType)
{
	uint8_t maxPorts = 0;

 	// Cable
	if (ruleType == USBCABLE_RULE) {
			Serial.print("USB MIDI CABLE OUT ROUTING");
			maxPorts = USBCABLE_INTERFACE_MAX;
	}

	// Serial
	else
	if (ruleType == SERIAL_RULE) {
			Serial.print("MIDI IN JACK ROUTING ");
			maxPorts = SERIAL_INTERFACE_COUNT;

	}
  else
	// IntelliThru
	if (ruleType == INTELLITHRU_RULE) {
			Serial.print("MIDI IN JACK INTELLITHRU ROUTING ");
			maxPorts = SERIAL_INTERFACE_COUNT;
	}
	else return;

	Serial.print(" (");Serial.print(maxPorts);
	Serial.println(" port(s) found)");

	Serial.println();
	Serial.print("|");
	Serial.print(ruleType == USBCABLE_RULE ? "Cb":"Jk");
	Serial.println("| Msg Filter   | Cable IN 1111111 | Jack OUT 1111111 |");
	Serial.println("|  | Ch Sc Rt Sx  | 1234567890123456 | 1234567890123456 |");

	for (uint8_t p=0; p< maxPorts ; p++) {

		void *anyRule;

		if (ruleType == USBCABLE_RULE) anyRule = (void *)&EEPROM_Params.midiRoutingRulesCable[p];
		else if (ruleType == SERIAL_RULE) anyRule = (void *)&EEPROM_Params.midiRoutingRulesSerial[p];
		else if (ruleType == INTELLITHRU_RULE) anyRule = (void *)&EEPROM_Params.midiRoutingRulesIntelliThru[p];
		else return;

		ShowMidiRoutingLine(p,ruleType, anyRule);

	} // for p

	Serial.println();
	if (ruleType == INTELLITHRU_RULE) {
		Serial.print("IntelliThru mode is ");
		Serial.println(EEPROM_Params.intelliThruJackInMsk ? "active." : "inactive.");
		Serial.print("USB sleeping detection time :");
    Serial.print(EEPROM_Params.intelliThruDelayPeriod*15);Serial.println("s");
	}

}

///////////////////////////////////////////////////////////////////////////////
// Show MidiKlik Header on screen
///////////////////////////////////////////////////////////////////////////////
static void ShowMidiKliKHeader()
{
  Serial.print("USBMIDIKLIK 4x4 - ");
	Serial.print(HARDWARE_TYPE);
	Serial.print(" - V");
	Serial.print(VERSION_MAJOR);Serial.print(".");Serial.println(VERSION_MINOR);
	Serial.println("(c) TheKikGen Labs");
	Serial.println("https://github.com/TheKikGen/USBMidiKliK4x4");
}

///////////////////////////////////////////////////////////////////////////////
// Show current EEPROM settings
///////////////////////////////////////////////////////////////////////////////
static void ShowGlobalSettings()
{
	uint8_t i,j;

	Serial.println("GLOBAL SETTINGS");
	Serial.println();
	Serial.print("Firmware version    : ");
	Serial.print(EEPROM_Params.majorVersion);Serial.print(".");
	Serial.println(EEPROM_Params.minorVersion);
	Serial.print("Magic number        : ");
	Serial.write(EEPROM_Params.signature , sizeof(EEPROM_Params.signature));
	Serial.println(EEPROM_Params.prmVersion);

	Serial.print("Build               : ");
	Serial.println( (char *)EEPROM_Params.TimestampedVersion);

	Serial.print("Sysex header        : ");
	for (i=0; i < sizeof(sysExInternalHeader); i++) {
			Serial.print(sysExInternalHeader[i],HEX);Serial.print(" ");
	}
	Serial.println();
	Serial.print("Next BootMode       : ");
	Serial.println(EEPROM_Params.nextBootMode);

	Serial.print("Debug Mode          : ");
	Serial.println(EEPROM_Params.debugMode ? "ENABLED":"DISABLED");

	Serial.print("Harware type        : ");
	Serial.println(HARDWARE_TYPE);

	Serial.print("EEPROM param. size  : ");
	Serial.println(sizeof(EEPROM_Params_t));

	Serial.print("I2C Bus mode        : ");
	if ( EEPROM_Params.I2C_BusModeState == B_DISABLED )
			Serial.println("DISABLED");
  else
			Serial.println("ENABLED");

	Serial.print("I2C Device ID       : ");
	Serial.print(EEPROM_Params.I2C_DeviceId);
	if ( EEPROM_Params.I2C_DeviceId == B_MASTERID )
			Serial.print(" (master)");
	else
			Serial.print(" (slave)");
	Serial.println();

	Serial.print("MIDI USB ports      : ");
	Serial.println(USBCABLE_INTERFACE_MAX);

	Serial.print("MIDI serial ports   : ");
	Serial.println(SERIAL_INTERFACE_COUNT);

	Serial.print("USB VID - PID       : ");
	Serial.print( EEPROM_Params.vendorID,HEX);
	Serial.print(" - ");
	Serial.println( EEPROM_Params.productID,HEX);

	Serial.print("USB Product string  : ");
	Serial.write(EEPROM_Params.productString, sizeof(EEPROM_Params.productString));
	Serial.println();
}

///////////////////////////////////////////////////////////////////////////////
// Setup a Midi routing target on screen
///////////////////////////////////////////////////////////////////////////////
uint16_t AskMidiRoutingTargets(uint8_t ruleType,uint8_t ruleTypeOut, uint8_t port)
{
	uint16_t targetsMsk = 0;
	uint8_t  portMax = (ruleTypeOut == USBCABLE_RULE? USBCABLE_INTERFACE_MAX:SERIAL_INTERFACE_COUNT);
	uint8_t choice;

	if (ruleType == INTELLITHRU_RULE && ruleTypeOut == USBCABLE_RULE ) return 0;

	for ( uint8_t i=0 ; i< portMax  ; i++ ){
		Serial.print("Route ");
		Serial.print(ruleType == USBCABLE_RULE ? "Cable OUT#":"Jack IN#");
		if (port+1 < 10) Serial.print("0");
		Serial.print(port+1);
		Serial.print(" to ");
		Serial.print(ruleTypeOut == USBCABLE_RULE ? "Cable IN#":"Jack OUT#");
		if (i+1 < 10) Serial.print("0");
		Serial.print(i+1);
		choice = AskChoice("  (x to exit)","ynx");
		if (choice == 'x') {Serial.println(); break;}
		else if ( choice == 'y') targetsMsk |= (1 << i);
		Serial.println();
	}
	return targetsMsk;
}

///////////////////////////////////////////////////////////////////////////////
// Setup a Midi routing target on screen
///////////////////////////////////////////////////////////////////////////////
void AskMidiRouting(uint8_t ruleType)
{
	uint16_t targetsMsk = 0;
	uint8_t  portMax = (ruleType == USBCABLE_RULE? USBCABLE_INTERFACE_MAX:SERIAL_INTERFACE_COUNT);
	uint8_t  choice;

	Serial.print("-- Set ");
	Serial.print(ruleType == USBCABLE_RULE ? "USB Cable OUT":"Jack IN");
	if (ruleType == INTELLITHRU_RULE ) Serial.print(" IntelliThru ");
	Serial.println(" routing --");

	Serial.print("Enter ");
	Serial.print(ruleType == USBCABLE_RULE ? "Cable OUT":"Jack IN");
	Serial.print (" # (01-");
	Serial.print( portMax > 9 ? "":"0");
	Serial.print( portMax);
	Serial.print(" / 00 to exit) :");
	while (1) {
		choice = AsknNumber(2);
		Serial.println();
		if ( choice > 0 && choice > portMax) {
			Serial.print("# error. Please try again :");
		} else break;
	}

	if (choice > 0) {
		uint8_t port =  choice - 1;
		Serial.println();

		if (ruleType == INTELLITHRU_RULE ) {
			Serial.print("Jack #");
			Serial.print( port+1 > 9 ? "":"0");Serial.print(port+1);
			if ( AskChoice(" : enable thru mode routing","") != 'y') {
				Serial.println();
				Serial.print("Jack IN #");Serial.print(port+1);
				Serial.println(" disabled.");
				EEPROM_Params.intelliThruJackInMsk &= ~(1 << port);
				return;
			}
			EEPROM_Params.intelliThruJackInMsk |= (1 << port);
			Serial.println();
			if ( EEPROM_Params.midiRoutingRulesIntelliThru[port].jackOutTargetsMsk )
			{
				if ( AskChoice("Keep existing Midi routing & filtering","") == 'y') {
					Serial.println();
					return;
				}
				Serial.println();
			}
			Serial.println();
		}

		// Filters
		uint8_t flt = AskMidiFilter(ruleType,port);
		if ( flt == 0 ) {
			Serial.println("No filters set. No change made.");
			return;
		}
		Serial.println();

		uint16_t cTargets=0;
		uint16_t jTargets=0;

		if (ruleType != INTELLITHRU_RULE ) {
			// Cable
			cTargets = AskMidiRoutingTargets(ruleType,USBCABLE_RULE,port);
			Serial.println();
		}

		// Midi jacks
		jTargets = AskMidiRoutingTargets(ruleType,SERIAL_RULE,port);

		if ( jTargets + cTargets == 0 ) {
			Serial.println("No targets set. No change made.");
			return;
		}

		if (ruleType == USBCABLE_RULE ) {
			EEPROM_Params.midiRoutingRulesCable[port].filterMsk = flt;
			EEPROM_Params.midiRoutingRulesCable[port].cableInTargetsMsk = cTargets;
			EEPROM_Params.midiRoutingRulesCable[port].jackOutTargetsMsk = jTargets ;
		} else
		if (ruleType == SERIAL_RULE ) {
			EEPROM_Params.midiRoutingRulesSerial[port].filterMsk = flt;
			EEPROM_Params.midiRoutingRulesSerial[port].cableInTargetsMsk = cTargets;
			EEPROM_Params.midiRoutingRulesSerial[port].jackOutTargetsMsk = jTargets ;
		} else
		if (ruleType == INTELLITHRU_RULE ) {
			EEPROM_Params.midiRoutingRulesIntelliThru[port].filterMsk = flt;
			EEPROM_Params.midiRoutingRulesIntelliThru[port].jackOutTargetsMsk = jTargets ;
		}

	}
}

///////////////////////////////////////////////////////////////////////////////
// Setup a Midi filter routing  on screen
///////////////////////////////////////////////////////////////////////////////
uint8_t AskMidiFilter(uint8_t ruleType, uint8_t port)
{
	uint8_t flt = 0;

	Serial.print("Set filter Midi messages for ");

	 if (ruleType == USBCABLE_RULE ) {
			 Serial.print("cable out #");
	 }
	 else
	 if (ruleType == SERIAL_RULE ||  ruleType == INTELLITHRU_RULE  ) {
			 Serial.print("MIDI in jack #");
	 }
	 else return 0;

	if (port+1 < 10) Serial.print ("0");
	Serial.print(port+1);
  Serial.println(" :");

	if ( AskChoice("Channel Voice   ","") == 'y')
			flt |= midiXparser::channelVoiceMsgTypeMsk;
	Serial.println();

	if ( AskChoice("System Common   ","") == 'y')
			flt |= midiXparser::systemCommonMsgTypeMsk;
	Serial.println();

	if ( AskChoice("Realtime        ","") == 'y' )
			flt |= midiXparser::realTimeMsgTypeMsk;
	Serial.println();

	if ( AskChoice("System Exclusive","") == 'y')
			flt |= midiXparser::sysExMsgTypeMsk;
	Serial.println();

	return flt;
}

///////////////////////////////////////////////////////////////////////////////
// Setup Product String on screen
///////////////////////////////////////////////////////////////////////////////
void AskProductString()
{
	uint8_t i = 0;
	char c;
	char buff [32];

	Serial.println("Enter product string - ENTER to terminate :");
	while ( i < USB_MIDI_PRODUCT_STRING_SIZE && (c = AskChar() ) !=13 ) {
		if ( c >= 32 && c < 127 ) {
			Serial.write(c);
			buff[i++]	= c;
		}
	}
	if ( i > 0 ) {
		memset(EEPROM_Params.productString,0,sizeof(EEPROM_Params.productString) );
		memcpy(EEPROM_Params.productString,buff,i);
	}

}

///////////////////////////////////////////////////////////////////////////////
// Setup PID and VID on screen
///////////////////////////////////////////////////////////////////////////////
void AskVIDPID()
{
	char buff [5];
	Serial.println("Enter VID - PID, in hex (0-9,a-f) :");
	AsknHexChar( (char *) buff, 4, 0, 0);
	EEPROM_Params.vendorID  = GetInt16FromHex4Bin((char*)buff);
	Serial.print("-");
	AsknHexChar( (char * ) buff, 4, 0, 0);
	EEPROM_Params.productID = GetInt16FromHex4Bin((char*)buff);
}

///////////////////////////////////////////////////////////////////////////////
// INFINITE LOOP - CONFIG ROOT MENU
//----------------------------------------------------------------------------
// Allow USBMidLKLIK configuration by using a menu as a user interface
// from the USB serial port.
///////////////////////////////////////////////////////////////////////////////
void ShowConfigMenu()
{
	char choice=0;
	uint8_t i;
  boolean showMenu = true;

	for ( ;; )
	{
		if (showMenu) {
  		ShowMidiKliKHeader();
      Serial.println();
			Serial.println("0.Global settings      6.IntelliThru routing");
			Serial.println("1.View midi routing    7.IntelliThru timeout");
  		Serial.println("2.USB VID PID          8.Toggle bus mode");
			Serial.println("3.USB Prod.string      9.Set device Id");
  		Serial.println("4.Cable OUT routing    a.Show active devices");
			Serial.println("5.Jack IN routing");
      Serial.println();
      Serial.println("e.Reload settings      f.Factory settings");
  		Serial.println("r.Factory routing      s.Save settings");
  		Serial.println("z.Debug on Serial3     x.Exit");

		}
    showMenu = true;
		Serial.println();
		Serial.print("=>");
		choice = AskChar();
		Serial.println(choice);
    Serial.println();

		switch (choice)
		{
			// Show global settings
			case '0':
				ShowGlobalSettings();
				Serial.println();
        showMenu = false;
			  break;

			// Show midi routing
			case '1':
			  ShowMidiRouting(USBCABLE_RULE);
				Serial.println();
			  ShowMidiRouting(SERIAL_RULE);
				Serial.println();
			  ShowMidiRouting(INTELLITHRU_RULE);
				Serial.println();
	      showMenu = false;
			  break;

			// Change VID & PID
			case '2':
				AskVIDPID();
				Serial.println();
				showMenu = false;
				break;

			// Change the product string
			case '3':
				AskProductString();
				Serial.println();
				showMenu = false;
				break;

			// Cables midi routing
			case '4':
				AskMidiRouting(USBCABLE_RULE);
				Serial.println();
				break;

			// Jack IN midi routing
			case '5':
				AskMidiRouting(SERIAL_RULE);
				Serial.println();
				break;

			// IntelliThru IN midi routing
			case '6':
				AskMidiRouting(INTELLITHRU_RULE);
				Serial.println();
				break;

			// USB Timeout <number of 15s periods 1-127>
			case '7':
				Serial.println("Nb of 15s periods (001-127 / 000 to exit) :");
				i = AsknNumber(3);
				if (i == 0 || i >127 )
					Serial.println(". Error. No change made.");
				else {
					EEPROM_Params.intelliThruDelayPeriod = i;
					Serial.print(" <= Delay set to ");Serial.print(i*15);Serial.println("s");
				}
				Serial.println();
				showMenu = false;
				break;

				// Enable Bus mode
			 case '8':
					if ( AskChoice("Enable bus mode","") == 'y' ) {
						EEPROM_Params.I2C_BusModeState = B_ENABLED;
						Serial.println();
						Serial.println("Bus mode enabled.");
						Serial.println();
					} else {
						EEPROM_Params.I2C_BusModeState = B_DISABLED;
						Serial.println();
						Serial.println("Bus mode disabled.");
						Serial.println();
					}
				  showMenu = false;
					break;

				// Set device ID.
			 case '9':
			 			Serial.print("Device ID ( ");
						Serial.print(B_MASTERID < 9 ? "0": "");
						Serial.print(B_MASTERID);
						Serial.print(":Master, Slaves ");
						Serial.print(B_SLAVE_DEVICE_BASE_ADDR < 9 ? "0": "");
						Serial.print(B_SLAVE_DEVICE_BASE_ADDR);
						Serial.print("-");
						Serial.print(B_SLAVE_DEVICE_LAST_ADDR < 9 ? "0": "");
						Serial.print(B_SLAVE_DEVICE_LAST_ADDR);
						Serial.print(") :");
						while (1) {
							i = AsknNumber(2);
							if ( i != B_MASTERID ) {
								if ( i > B_SLAVE_DEVICE_LAST_ADDR || i < B_SLAVE_DEVICE_BASE_ADDR ) {
										Serial.println();
										Serial.print("Id error. Try again :");
								} else break;
							} else break;
						}
						EEPROM_Params.I2C_DeviceId = i;
						Serial.println();
						Serial.println();
						showMenu = false;

					break;

			// Show active devices on the I2C Bus
			case 'a':
				Serial.println();
				I2C_ShowActiveDevice();
				Serial.println();
				showMenu = false;
				break;

			// Reload the EEPROM parameters structure
			case 'e':
				if ( AskChoice("Reload previous settings ","") == 'y' ) {
					EEPROM_ParamsInit();
          Serial.println();
					Serial.println("Settings reloaded.");
					Serial.println();
				}
				showMenu = false;
				break;

			// Restore factory settings
			case 'f':
				if ( AskChoice("Restore factory settings","") == 'y' ) {
          Serial.println();
					if ( AskChoice("All settings will be erased. Sure","") == 'y' ) {
						EEPROM_ParamsInit(true);
            Serial.println();
						Serial.println("Factory settings restored.");
						Serial.println();
					}
				}
				showMenu = false;
				break;

			// Default midi routing
			case 'r':
				if ( AskChoice("Reset Midi routing to default","") == 'y')
					ResetMidiRoutingRules(ROUTING_RESET_ALL);
					Serial.println();
					showMenu = false;
				break;

			// Save & quit
			case 's':
				ShowGlobalSettings();
				Serial.println();
				if ( AskChoice("Save settings","") == 'y' ) {
					Serial.println();
					//Goto midi mode at the next boot
					EEPROM_Params.nextBootMode = bootModeMidi;
					EEPROM_ParamsSave();
					delay(100);
					showMenu = false;
				}
				break;

        // debug Mode
  			case 'z':
        #ifdef DEBUG_MODE
        if ( AskChoice("Enable debug mode.","") == 'y' ) {
          EEPROM_Params.debugMode = true;
          Serial.println();
          Serial.println("Debug mode enabled. M:Serial3, S:UsbSerial. 115200 bauds");
          Serial.println();
        } else {
          EEPROM_Params.debugMode = false;
          Serial.println();
          Serial.println("Debug mode disabled.");
          Serial.println();
        }
        #else
          Serial.println("Not available in that firmware.");
          Serial.println();
        #endif
				showMenu = false;
        break;

			// Abort
			case 'x':
				if ( AskChoice("Exit","") == 'y' ) {
            Serial.println();
						Serial.end();
						nvic_sys_reset();
				}
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Check what is the current boot mode.
// Will never come back if config mode.
///////////////////////////////////////////////////////////////////////////////
void CheckBootMode()
{
	// Does the config menu boot mode is active ?
	// if so, prepare the next boot in MIDI mode and jump to menu
	if  ( EEPROM_Params.nextBootMode == bootModeConfigMenu ) {

			#ifdef HAS_MIDITECH_HARDWARE
				// Assert DISC PIN (PA8 usually for Miditech) to enable USB
				gpio_set_mode(PIN_MAP[PA8].gpio_device, PIN_MAP[PA8].gpio_bit, GPIO_OUTPUT_PP);
				gpio_write_bit(PIN_MAP[PA8].gpio_device, PIN_MAP[PA8].gpio_bit, 1);
			#endif

			// start USB serial
			Serial.begin(115200);
			delay(500);

			// Start Wire as a master for config & tests purpose in the menu
			Wire.begin();
			Wire.setClock(B_FREQ) ;

			// wait for a serial monitor to be connected.
			// 3 short flash

			while (!Serial) {
					flashLED_CONNECT->start();delay(100);
					flashLED_CONNECT->start();delay(100);
					flashLED_CONNECT->start();delay(300);
				}
			digitalWrite(LED_CONNECT, LOW);
			ShowConfigMenu(); // INFINITE LOOP
	}
}

///////////////////////////////////////////////////////////////////////////////
// MIDI USB initiate connection if master
// + Set USB descriptor strings
///////////////////////////////////////////////////////////////////////////////
void USBMidi_Init()
{
	usb_midi_set_vid_pid(EEPROM_Params.vendorID,EEPROM_Params.productID);
	usb_midi_set_product_string((char *) &EEPROM_Params.productString);

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

    if (! midiUSBCx) digitalWrite(LED_CONNECT, LOW);
    midiUSBCx = true;

		// Do we have a MIDI USB packet available ?
		if ( MidiUSB.available() ) {
			midiUSBLastPacketMillis = millis()  ;
			midiUSBIdle = false;
			intelliThruActive = false;

			// Read a Midi USB packet .
			if ( !isSerialBusy ) {
				midiPacket_t pk ;
				pk.i = MidiUSB.readPacket();
				RoutePacketToTarget( FROM_USB,  &pk );
			} else {
					isSerialBusy = false ;
			}
		} else
		if (!midiUSBIdle && millis() > ( midiUSBLastPacketMillis + intelliThruDelayMillis ) )
				midiUSBIdle = true;
	}
	// Are we physically connected to USB
	else {
       if (midiUSBCx) digitalWrite(LED_CONNECT, HIGH);
       midiUSBCx = false;
		   midiUSBIdle = true;
  }

	if ( midiUSBIdle && !intelliThruActive && EEPROM_Params.intelliThruJackInMsk) {
			intelliThruActive = true;
			FlashAllLeds(0); // All leds when Midi intellithru mode active
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
					 if ( midiSerial[s].parse( serialHw[s]->read() ) ) {
								// We manage sysEx "on the fly". Clean end of a sysexe msg ?
								if ( midiSerial[s].getMidiMsgType() == midiXparser::sysExMsgTypeMsk )
									SerialMidi_RouteSysEx(s, &midiSerial[s]) ;

								// Not a sysex. The message is complete.
								else {
                  SerialMidi_RouteMsg( s, &midiSerial[s]);
                }

					 }
					 else
					 // Acknowledge any sysex error
					 if ( midiSerial[s].isSysExError() )
						 SerialMidi_RouteSysEx(s, &midiSerial[s]) ;
					 else
					 // Check if a SYSEX mode active and send bytes on the fly.
					 if ( midiSerial[s].isSysExMode() && midiSerial[s].isByteCaptured() ) {
							SerialMidi_RouteSysEx(s, &midiSerial[s]) ;
					 }
				}

				// Manage Serial contention vs USB
				// When one or more of the serial buffer is full, we block USB read one round.
				// This implies to use non blocking Serial.write(buff,len).
				if (  midiUSBCx &&  !serialHw[s]->availableForWrite() ) isSerialBusy = true; // 1 round without reading USB
	}
}

///////////////////////////////////////////////////////////////////////////////
// I2C Get a master packet from slave
///////////////////////////////////////////////////////////////////////////////
int16_t I2C_getPacket(uint8_t deviceId, masterMidiPacket_t *mpk)
{
    uint8_t nb = I2C_SendCommand(deviceId,B_CMD_GET_MPACKET);
    if ( nb > 0 ) Wire.readBytes( mpk->packet,sizeof(masterMidiPacket_t)) ;

DEBUG_BEGIN
DEBUG_PRINT("GetPk (","");
DEBUG_DUMP(mpk->packet,sizeof(masterMidiPacket_t));
DEBUG_END

    return (nb) ;
}

///////////////////////////////////////////////////////////////////////////////
// I2C check if a device is active on the bus
//////////////////////////////////////////////////////////////////////////////
boolean I2C_isDeviceActive(uint8_t deviceId)
{
    Wire.beginTransmission(deviceId);
    return Wire.endTransmission() == 0;
}

///////////////////////////////////////////////////////////////////////////////
// I2C Send bulk data on the bus.
//////////////////////////////////////////////////////////////////////////////
int8_t I2C_SendData(const uint8_t dataType, uint8_t arg1, uint8_t arg2, uint8_t * data, uint16_t sz)
{

  if (sz > 29 ) return -1; // Wire buffer is limiter to 32 char. Enough for us.

  uint8_t *p = data;

  Wire.beginTransmission(0); // Broadcast
  Wire.write(dataType);
  Wire.write(arg1);
  Wire.write(arg2);
  Wire.write(data,sz);
  if ( Wire.endTransmission()) return -1;
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// I2C Slaves SYNC routing rules at boot
///////////////////////////////////////////////////////////////////////////////
void I2C_SlavesRoutingSyncFromMaster()
{
  I2C_SendCommand(0,   B_CMD_START_SYNC);

  // Send midiRoutingRulesCable
  for ( uint8_t i=0 ; i< USBCABLE_INTERFACE_MAX ; i ++ ) {
    I2C_SendData(B_DTYPE_MIDI_ROUTING_RULES_CABLE, i, 0, (uint8_t *)&EEPROM_Params.midiRoutingRulesCable[i], sizeof(midiRoutingRule_t));
  }

  // Send midiRoutingRulesSerial -  midiRoutingRulesIntelliThru
  for ( uint8_t i=0 ; i< B_SERIAL_INTERFACE_MAX ; i ++ ) {
    I2C_SendData(B_DTYPE_MIDI_ROUTING_RULES_SERIAL, i, 0, (uint8_t *)&EEPROM_Params.midiRoutingRulesSerial[i], sizeof(midiRoutingRule_t));
    I2C_SendData(B_DTYPE_MIDI_ROUTING_RULES_INTELLITHRU, i, 0, (uint8_t *)&EEPROM_Params.midiRoutingRulesIntelliThru[i], sizeof(midiRoutingRuleJack_t));
  }

  I2C_SendData(B_DTYPE_MIDI_ROUTING_INTELLITHRU_JACKIN_MSK, 0, 0,(uint8_t *)&EEPROM_Params.intelliThruJackInMsk, sizeof(EEPROM_Params.intelliThruJackInMsk));
  I2C_SendData(B_DTYPE_MIDI_ROUTING_INTELLITHRU_DELAY_PERIOD, 0, 0, (uint8_t *)&EEPROM_Params.intelliThruDelayPeriod, sizeof(EEPROM_Params.intelliThruDelayPeriod));

  I2C_SendCommand(0,   B_CMD_END_SYNC);

}

///////////////////////////////////////////////////////////////////////////////
// I2C Loop Process for a MASTER
///////////////////////////////////////////////////////////////////////////////
void I2C_ProcessMaster ()
{
  masterMidiPacket_t mpk;
  uint8_t deviceId;

  // Broadcast slaves of USB midi state & intellithru mode
  I2C_SendCommand(0, midiUSBCx ?  B_CMD_USBCX_AVAILABLE:B_CMD_USBCX_UNAVAILABLE);
  I2C_SendCommand(0, midiUSBIdle ?  B_CMD_USBCX_SLEEP:B_CMD_USBCX_AWAKE );
  I2C_SendCommand(0, intelliThruActive ?  B_CMD_INTELLITHRU_ENABLED:B_CMD_INTELLITHRU_DISABLED ) ;

  // Notify slaves of debug mode. The debug mode of the slave is overwriten
  #ifdef DEBUG_MODE
  I2C_SendCommand(0, EEPROM_Params.debugMode ?  B_CMD_DEBUG_MODE_ENABLED:B_CMD_DEBUG_MODE_DISABLED ) ;
  #endif

  for ( uint8_t d = 0 ; d < sizeof(I2C_DeviceActive) ; d++) {

      if ( ! I2C_DeviceActive[d] )  continue;
      deviceId = d+B_SLAVE_DEVICE_BASE_ADDR;

      // Get a slave midi packet eventually
      if ( I2C_SendCommand(deviceId,B_CMD_ISPACKET_AVAIL) <= 0) continue;  // No packets or error
      if ( I2C_getPacket(deviceId,&mpk) <= 0 ) continue; // Error or nothing got
      if ( mpk.mpk.pk.i == 0 ) continue;  // Process only non empty packets

      if ( mpk.mpk.dest == TO_SERIAL ) {
          uint8_t targetPort = mpk.mpk.pk.packet[0] >> 4;
          I2C_BusSerialSendMidiPacket(&(mpk.mpk.pk), targetPort );

DEBUG_BEGIN
DEBUG_PRINTLN(" Sent to BUS serial. target port ",targetPort);
DEBUG_END

         // Send to a cable IN only if USB is available on the master
       } else if ( mpk.mpk.dest == TO_USB )
        //&& midiUSBCx && !midiUSBdle && !intelliThruActive)
       {

          MidiUSB.writePacket(&(mpk.mpk.pk.i));

DEBUG_BEGIN
DEBUG_PRINTLN(" Sent to USB","");
DEBUG_END

       }
      // else ignore that packet....

	} // for
}

///////////////////////////////////////////////////////////////////////////////
// I2C Loop Process for a SLAVE
///////////////////////////////////////////////////////////////////////////////
void I2C_ProcessSlave ()
{

  I2C_MasterReady = ( millis() < (I2C_MasterReadyTimeoutMillis + B_MASTER_READY_TIMEOUT) );

	// Packet from I2C master available ? (nb : already routed)
	if ( I2C_QPacketsFromMaster.available() ) {
			midiPacket_t pk;
			// These packets are mandatory local as a result of the routing
			I2C_QPacketsFromMaster.readBytes(pk.packet,sizeof(midiPacket_t));

DEBUG_BEGIN
DEBUG_PRINT("Read Master Q :","");
DEBUG_DUMP(pk.packet,sizeof(midiPacket_t));
DEBUG_PRINTLN(". count=",I2C_QPacketsFromMaster.available()/sizeof(midiPacket_t));
DEBUG_END

			SerialMidi_SendPacket(&pk, (pk.packet[0] >> 4) % SERIAL_INTERFACE_MAX );

DEBUG_BEGIN
DEBUG_PRINTLN(" Sent to local serial ",(pk.packet[0] >> 4) % SERIAL_INTERFACE_MAX );
DEBUG_END

	}

	// Activate the configuration menu if a terminal is opened in Slave mode
  // and C pressed
	if (Serial.available()) {
      uint8_t key = Serial.read();

      if ( key == 'C') {
        Wire.flush();
        Wire.end();
        ShowConfigMenu();
      }
      else if ( key == 'R') {
        Serial.println();
        ShowMidiRouting(USBCABLE_RULE);
        Serial.println();
        ShowMidiRouting(SERIAL_RULE);
        Serial.println();
        ShowMidiRouting(INTELLITHRU_RULE);
        Serial.println();
      }
	}

DEBUG_BEGIN_TIMER
DEBUG_ASSERT(!I2C_MasterReady,"Master not ready","");
DEBUG_ASSERT(midiUSBCx,"USB Midi Cx alive","");
DEBUG_ASSERT(midiUSBCx,"USB Midi Cx alive","");
DEBUG_ASSERT(midiUSBIdle,"USB Midi idle","");
DEBUG_ASSERT(intelliThruActive,"Intellithru active","");
DEBUG_END

}

///////////////////////////////////////////////////////////////////////////////
// SETUP
///////////////////////////////////////////////////////////////////////////////
void setup()
{

		// EEPROM initialization
		// Set EEPROM parameters for the STMF103RC
	  EEPROM.PageBase0 = 0x801F000;
	  EEPROM.PageBase1 = 0x801F800;
	  EEPROM.PageSize  = 0x800;
		//EEPROM.Pages     = 1;


		EEPROM.init();

		// Retrieve EEPROM parameters
    EEPROM_ParamsInit();

    #ifndef DEBUG_MODE
    EEPROM_Params.debugMode = false;
    #endif


    // Configure the TIMER2
    timer.pause();
    timer.setPeriod(TIMER2_RATE_MICROS); // in microseconds
    // Set up an interrupt on channel 1
    timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
    timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
    timer.attachCompare1Interrupt(Timer2Handler);
    timer.refresh();   // Refresh the timer's count, prescale, and overflow
    timer.resume();    // Start the timer counting

    // Start the LED Manager
    flashLEDManager.begin();

		CheckBootMode();

    // MIDI MODE START HERE ==================================================

    intelliThruDelayMillis = EEPROM_Params.intelliThruDelayPeriod * 15000;

    // MIDI SERIAL PORTS set Baud rates and parser inits
    // To compile with the 4 serial ports, you must use the right variant : STMF103RC
    // + Set parsers filters in the same loop.  All messages including on the fly SYSEX.

    for ( uint8_t s=0; s < SERIAL_INTERFACE_MAX ; s++ ) {
      serialHw[s]->begin(31250);
      midiSerial[s].setMidiMsgFilter( midiXparser::allMsgTypeMsk );
    }

		// I2C bus checks that could disable the bus mode
  	I2C_BusChecks();

    // Midi USB only if master when bus is enabled or master/slave
    if ( ! B_IS_SLAVE  ) {
        midiUSBLaunched = true;
        USBMidi_Init();
        #ifdef DEBUG_MODE
        if (EEPROM_Params.debugMode ) {
            DEBUG_SERIAL.end();
            DEBUG_SERIAL.begin(115200);
            DEBUG_SERIAL.flush();
            delay(500);
        }
        #endif
  	}

		I2C_BusStartWire();		// Start Wire if bus mode enabled. AFTER MIDI !

}

///////////////////////////////////////////////////////////////////////////////
// LOOP
///////////////////////////////////////////////////////////////////////////////
void loop()
{
		if ( midiUSBLaunched ) USBMidi_Process();

		SerialMidi_Process();

		// I2C BUS MIDI PACKET PROCESS
		if ( B_IS_SLAVE ) I2C_ProcessSlave();
		else if ( B_IS_MASTER ) I2C_ProcessMaster();
}
