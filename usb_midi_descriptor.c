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
  DEVICE DESCRIPTOR

  USB MIDI LIBRARY adapted by TheKikGenLab from USB LeafLabs LLC. USB API :
  Perry Hung, Magnus Lundin,Donald Delmar Davis, Suspect Devices.
  ----------------------------------------------------------------------

*/

// ---------------------------------------------------------------
// Full assembled USB Descriptor
// ---------------------------------------------------------------

#include "usb_midi_device.h"

// ---------------------------------------------------------------
// DEVICE DESCRIPTOR
// ---------------------------------------------------------------
static usb_descriptor_device usbMIDIDescriptor_Device = {
      .bLength            = sizeof(usb_descriptor_device),
      .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,
      .bcdUSB             = 0x0110,
      .bDeviceClass       = USB_DEVICE_CLASS_UNDEFINED,
      .bDeviceSubClass    = USB_DEVICE_SUBCLASS_UNDEFINED,
      .bDeviceProtocol    = 0x00,
      .bMaxPacketSize0    = USB_MIDI_MAX_PACKET_SIZE,
      .idVendor           = USB_MIDI_VENDORID,
      .idProduct          = USB_MIDI_PRODUCTID,
      .bcdDevice          = 0x0100,
      .iManufacturer      = 0x01,
      .iProduct           = 0x02,
      .iSerialNumber      = 0x00,
      .bNumConfigurations = 0x01,
 };

// ---------------------------------------------------------------
// CONFIGURATION DESCRIPTOR
// ---------------------------------------------------------------


typedef struct {
    usb_descriptor_config_header       Config_Header;
    /* Control Interface */
    usb_descriptor_interface           AC_Interface;
    AC_CS_INTERFACE_DESCRIPTOR(1)      AC_CS_Interface;
    /* Control Interface */
    usb_descriptor_interface           MS_Interface;
    MS_CS_INTERFACE_DESCRIPTOR         MS_CS_Interface;

    // MIDI IN DESCRIPTORS - 16 MAX
    // 1 PORT IS THE MINIMUM.

    // Embedded

    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_1;
    #if USB_MIDI_IO_PORT_NUM >= 2
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_2;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 3
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_3;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 4
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_4;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 5
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_5;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 6
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_6;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 7
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_7;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 8
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_8;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 9
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_9;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 10
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_A;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 11
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_B;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 12
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_C;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 13
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_D;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 14
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_E;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 15
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_F;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 16
    MIDI_IN_JACK_DESCRIPTOR            MIDI_IN_JACK_10;
    #endif

    // MIDI OUT DESCRIPTORS - 16 MAX
    // External
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_11;
    #if USB_MIDI_IO_PORT_NUM >= 2
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_12;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 3
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_13;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 4
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_14;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 5
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_15;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 6
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_16;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 7
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_17;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 8
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_18;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 9
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_19;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 10
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1A;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 11
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1B;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 12
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1C;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 13
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1D;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 14
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1E;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 15
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_1F;
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 16
    MIDI_OUT_JACK_DESCRIPTOR(1)        MIDI_OUT_JACK_20;
    #endif

    MIDI_USB_DESCRIPTOR_ENDPOINT       DataOutEndpoint;
    MS_CS_BULK_ENDPOINT_DESCRIPTOR(USB_MIDI_IO_PORT_NUM)  MS_CS_DataOutEndpoint;
} __packed usb_midi_descriptor_config;

static const usb_midi_descriptor_config usbMIDIDescriptor_Config = {
    .Config_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_midi_descriptor_config),
        .bNumInterfaces       = 0x02,
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = 0x80 , // (Bus Powered)
        // .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
        //                           USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = USB_MIDI_MAX_POWER,
    },

    .AC_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0x00,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x00,
        .bInterfaceClass    = USB_INTERFACE_CLASS_AUDIO,
        .bInterfaceSubClass = USB_INTERFACE_AUDIOCONTROL,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0x00,
    },

    .AC_CS_Interface = {
        .bLength            = AC_CS_INTERFACE_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = 0x01,
        .bcdADC             = 0x0100,
        .wTotalLength       = AC_CS_INTERFACE_DESCRIPTOR_SIZE(1),
        .bInCollection      = 0x01,
        .baInterfaceNr      = {0x01},
    },

    .MS_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = 0x01,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x01,
        .bInterfaceClass    = USB_INTERFACE_CLASS_AUDIO,
        .bInterfaceSubClass = USB_INTERFACE_MIDISTREAMING,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0x03, // Midi
    },

    .MS_CS_Interface = {
        .bLength            = sizeof(MS_CS_INTERFACE_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = 0x01,
        .bcdADC             = 0x0100,
        .wTotalLength       = sizeof(MS_CS_INTERFACE_DESCRIPTOR)
                              +USB_MIDI_IO_PORT_NUM*sizeof(MIDI_IN_JACK_DESCRIPTOR)
                              +USB_MIDI_IO_PORT_NUM*MIDI_OUT_JACK_DESCRIPTOR_SIZE(1)
                              +sizeof(MIDI_USB_DESCRIPTOR_ENDPOINT)
                              +MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(USB_MIDI_IO_PORT_NUM),
    },

    // Start of MIDI JACK Descriptor =======================================

    // MIDI IN JACK - EMBEDDED - 16 Descriptors -----------------------------
    .MIDI_IN_JACK_1 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x01,
        .iJack              = 0x04,  // Waveblaster  1
    },
    #if USB_MIDI_IO_PORT_NUM >= 2
    .MIDI_IN_JACK_2 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x02,
        .iJack              = 0x05,  // Waveblaster  2
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 3
    .MIDI_IN_JACK_3 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x03,
        .iJack              = 0x06,  // Waveblaster  3
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 4
    .MIDI_IN_JACK_4 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x04,
        .iJack              = 0x07,  // Waveblaster  4
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 5
    .MIDI_IN_JACK_5 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x05,
        .iJack              = 0x08,  // Waveblaster  5
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 6
    .MIDI_IN_JACK_6 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x06,
        .iJack              = 0x09,  // Waveblaster  6
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 7
    .MIDI_IN_JACK_7 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x07,
        .iJack              = 0x0A,  // Waveblaster  7
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 8
    .MIDI_IN_JACK_8 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x08,
        .iJack              = 0x0B,  // Waveblaster  8
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 9
    .MIDI_IN_JACK_9 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x09,
        .iJack              = 0x0C,  // Waveblaster  9
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 10
    .MIDI_IN_JACK_A = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0A,
        .iJack              = 0x0D,  // Waveblaster 10
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 11
    .MIDI_IN_JACK_B = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0B,
        .iJack              = 0x0E,  // Waveblaster 11
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 12
    .MIDI_IN_JACK_C = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0C,
        .iJack              = 0x0F,  // Waveblaster 12
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 13
    .MIDI_IN_JACK_D = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0D,
        .iJack              = 0x10,  // Waveblaster 13
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 14
    .MIDI_IN_JACK_E = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0E,
        .iJack              = 0x11,  // Waveblaster 14
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 15
    .MIDI_IN_JACK_F = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x0F,
        .iJack              = 0x12,  // Waveblaster 15
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 16
    .MIDI_IN_JACK_10 = {
        .bLength            = sizeof(MIDI_IN_JACK_DESCRIPTOR),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_IN_JACK,
        .bJackType          = MIDI_JACK_EMBEDDED,
        .bJackId            = 0x10,
        .iJack              = 0x13,  // Waveblaster 16
    },
    #endif

    // MIDI OUT JACK - EXTERNAL - 16 Descriptors -----------------------------

    .MIDI_OUT_JACK_11 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x11,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x01}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #if USB_MIDI_IO_PORT_NUM >= 2
    .MIDI_OUT_JACK_12 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x12,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x02}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 3
    .MIDI_OUT_JACK_13 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x13,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x03}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 4
    .MIDI_OUT_JACK_14 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x14,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x04}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 5
    .MIDI_OUT_JACK_15 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x15,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x05}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 6
    .MIDI_OUT_JACK_16 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x16,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x06}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 7
    .MIDI_OUT_JACK_17 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x17,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x07}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 8
    .MIDI_OUT_JACK_18 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x18,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x08}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 9
    .MIDI_OUT_JACK_19 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x19,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x09}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 10
    .MIDI_OUT_JACK_1A = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1A,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0A}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 11
    .MIDI_OUT_JACK_1B = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1B,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0B}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 12
    .MIDI_OUT_JACK_1C = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1C,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0C}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 13
    .MIDI_OUT_JACK_1D = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1D,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0D}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 14
    .MIDI_OUT_JACK_1E = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1E,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0E}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 15
    .MIDI_OUT_JACK_1F = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x1F,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x0F}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif
    #if USB_MIDI_IO_PORT_NUM >= 16
    .MIDI_OUT_JACK_20 = {
        .bLength            = MIDI_OUT_JACK_DESCRIPTOR_SIZE(1),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_CS_INTERFACE,
        .SubType            = MIDI_OUT_JACK,
        .bJackType          = MIDI_JACK_EXTERNAL,
        .bJackId            = 0x20,
        .bNrInputPins       = 0x01,
        .baSourceId         = {0x10}, // IN Embedded
        .baSourcePin        = {0x01},
        .iJack              = 0x00,
    },
    #endif

    // End of MIDI JACK Descriptor =======================================

    .DataOutEndpoint = {
        .bLength            = sizeof(MIDI_USB_DESCRIPTOR_ENDPOINT),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress   = (USB_DESCRIPTOR_ENDPOINT_OUT |
                             MIDI_STREAM_OUT_ENDP),
        .bmAttributes       = USB_EP_TYPE_BULK,
        .wMaxPacketSize     = MIDI_STREAM_EPSIZE,
        .bInterval          = 0x00,
        .bRefresh           = 0x00,
        .bSynchAddress      = 0x00,
    },

    .MS_CS_DataOutEndpoint = {
      .bLength              = MS_CS_BULK_ENDPOINT_DESCRIPTOR_SIZE(USB_MIDI_IO_PORT_NUM),
      .bDescriptorType      = USB_DESCRIPTOR_TYPE_CS_ENDPOINT,
      .SubType              = 0x01,
      // MIDI IN EMBEDDED
      .bNumEmbMIDIJack      = USB_MIDI_IO_PORT_NUM,
      .baAssocJackID        = {
        0x01,
        #if USB_MIDI_IO_PORT_NUM >= 2
        0X02,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 3
        0X03,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 4
        0X04,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 5
        0X05,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 6
        0X06,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 7
        0X07,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 8
        0X08,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 9
        0X09,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 10
        0X0A,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 11
        0X0B,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 12
        0X0C,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 13
        0X0D,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 14
        0X0E,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 15
        0X0F,
        #endif
        #if USB_MIDI_IO_PORT_NUM >= 16
        0X10,
        #endif

      },
  },

};
// --------------------------------------------------------------------------------------
//  String Descriptors:
// --------------------------------------------------------------------------------------

/*  we may choose to specify any or none of the following string
  identifiers:
  iManufacturer:    LeafLabs
  iProduct:         Maple
  iSerialNumber:    NONE
  iConfiguration:   NONE
  iInterface(CCI):  NONE
  iInterface(DCI):  NONE
*/

/* Unicode language identifier: 0x0409 is US English */
static const usb_descriptor_string usbMIDIDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

static const usb_descriptor_string usbMIDIDescriptor_iManufacturer = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(11),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
     // Open Source
     .bString = {'O', 0, 'p', 0, 'e', 0, 'n', 0,' ', 0, 'S', 0, 'o', 0, 'u', 0, 'r', 0, 'c', 0, 'e', 0},
};


// We reserve room to change the product string later but the lenght is manually adjusted to 11.
static usb_descriptor_string usbMIDIDescriptor_iProduct = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(11),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString =  { 'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0,  // 10
                  'r', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 , 0 , 0 , 0,  // 20
                   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 , 0,  // 30
                   // USB_MIDI_PRODUCT_STRING_SIZE = 30
                }
};

static const usb_descriptor_string usbMIDIDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(4),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'i', 0, 'd', 0, 'i', 0},
};

// Midi Jack 1
#if USB_MIDI_IO_PORT_NUM <= 1
static usb_descriptor_string usbMIDIDescriptor_iJack1 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(11),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0 },
};
#else
static usb_descriptor_string usbMIDIDescriptor_iJack1 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '1', 0 },
};
#endif

// Midi Jack 2
#if USB_MIDI_IO_PORT_NUM >= 2
static usb_descriptor_string usbMIDIDescriptor_iJack2 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '2', 0 },
};
#endif

// Midi Jack 3
#if USB_MIDI_IO_PORT_NUM >= 3
static usb_descriptor_string usbMIDIDescriptor_iJack3 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '3', 0 },
};
#endif

// Midi Jack 4
#if USB_MIDI_IO_PORT_NUM >= 4
static usb_descriptor_string usbMIDIDescriptor_iJack4 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '4', 0 },
};
#endif

// Midi Jack 5
#if USB_MIDI_IO_PORT_NUM >= 5
static usb_descriptor_string usbMIDIDescriptor_iJack5 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '5', 0 },
};
#endif

// Midi Jack 6
#if USB_MIDI_IO_PORT_NUM >= 6
static usb_descriptor_string usbMIDIDescriptor_iJack6 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '6', 0 },
};
#endif

// Midi Jack 7
#if USB_MIDI_IO_PORT_NUM >= 7
static usb_descriptor_string usbMIDIDescriptor_iJack7 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '7', 0 },
};
#endif

// Midi Jack 8
#if USB_MIDI_IO_PORT_NUM >= 8
static usb_descriptor_string usbMIDIDescriptor_iJack8 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '8', 0 },
};
#endif

// Midi Jack 9
#if USB_MIDI_IO_PORT_NUM >= 9
static usb_descriptor_string usbMIDIDescriptor_iJack9 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, ' ', 0, '9', 0 },
};
#endif

// Midi Jack 10
#if USB_MIDI_IO_PORT_NUM >= 10
static usb_descriptor_string usbMIDIDescriptor_iJackA = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '0', 0 },
};
#endif

// Midi Jack 11
#if USB_MIDI_IO_PORT_NUM >= 11
static usb_descriptor_string usbMIDIDescriptor_iJackB = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '1', 0 },
};
#endif

// Midi Jack 12
#if USB_MIDI_IO_PORT_NUM >= 12
static usb_descriptor_string usbMIDIDescriptor_iJackC = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '2', 0 },
};
#endif

// Midi Jack 13
#if USB_MIDI_IO_PORT_NUM >= 13
static usb_descriptor_string usbMIDIDescriptor_iJackD = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '3', 0 },
};
#endif

// Midi Jack 14
#if USB_MIDI_IO_PORT_NUM >= 14
static usb_descriptor_string usbMIDIDescriptor_iJackE = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '4', 0 },
};
#endif

// Midi Jack 15
#if USB_MIDI_IO_PORT_NUM >= 15
static usb_descriptor_string usbMIDIDescriptor_iJackF = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '5', 0 },
};
#endif

// Midi Jack 16
#if USB_MIDI_IO_PORT_NUM >= 16
static usb_descriptor_string usbMIDIDescriptor_iJack10 = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(14),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'W', 0, 'a', 0, 'v', 0, 'e', 0, 'b', 0, 'l', 0, 'a', 0, 's', 0, 't', 0 ,'e', 0, 'r', 0, ' ', 0, '1', 0, '6', 0 },
};
#endif



static ONE_DESCRIPTOR usbMidiDevice_Descriptor = {
    (uint8*)&usbMIDIDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR usbMidiConfig_Descriptor = {
    (uint8*)&usbMIDIDescriptor_Config,
    sizeof(usb_midi_descriptor_config)
};

#define USB_MIDI_N_STRING_DESCRIPTORS (4 + USB_MIDI_IO_PORT_NUM)
static ONE_DESCRIPTOR usbMIDIString_Descriptor[USB_MIDI_N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbMIDIDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1) },
    {(uint8*)&usbMIDIDescriptor_iManufacturer,USB_DESCRIPTOR_STRING_LEN(11)},
    {(uint8*)&usbMIDIDescriptor_iProduct,     USB_DESCRIPTOR_STRING_LEN(11)},
    {(uint8*)&usbMIDIDescriptor_iInterface,   USB_DESCRIPTOR_STRING_LEN(4) },
#if USB_MIDI_IO_PORT_NUM <= 1
    {(uint8*)&usbMIDIDescriptor_iJack1,       USB_DESCRIPTOR_STRING_LEN(11)},
#else
    {(uint8*)&usbMIDIDescriptor_iJack1,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 2
    {(uint8*)&usbMIDIDescriptor_iJack2,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 3
    {(uint8*)&usbMIDIDescriptor_iJack3,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 4
    {(uint8*)&usbMIDIDescriptor_iJack4,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 5
    {(uint8*)&usbMIDIDescriptor_iJack5,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 6
    {(uint8*)&usbMIDIDescriptor_iJack6,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 7
    {(uint8*)&usbMIDIDescriptor_iJack7,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 8
    {(uint8*)&usbMIDIDescriptor_iJack8,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 9
    {(uint8*)&usbMIDIDescriptor_iJack9,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 10
    {(uint8*)&usbMIDIDescriptor_iJackA,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 11
    {(uint8*)&usbMIDIDescriptor_iJackB,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 12
    {(uint8*)&usbMIDIDescriptor_iJackC,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 13
    {(uint8*)&usbMIDIDescriptor_iJackD,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 14
    {(uint8*)&usbMIDIDescriptor_iJackE,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 15
    {(uint8*)&usbMIDIDescriptor_iJackF,       USB_DESCRIPTOR_STRING_LEN(14)},
#endif
#if USB_MIDI_IO_PORT_NUM >= 16
    {(uint8*)&usbMIDIDescriptor_iJack10,      USB_DESCRIPTOR_STRING_LEN(14)},
#endif
};


