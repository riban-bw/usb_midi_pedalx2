/*  riban USB MIDI Soft and Sustain Pedal
    License: MIT
    Copyright riban ltd 2024
    This is the main source code file containing the firmware for STM32F103 (Bluepill)
*/

#include <Arduino.h>
#include <USBComposite.h>

#define EMA_A 0.2f // filter coeficient (0..1 higher value gives less agressive filter - 1.0 would pass all data, unfiltered)
#define ADC_BITS_TO_IGNORE 0 // Can reduce resolution if too noisy
#define LED PC13
#define DEADZONE 100 // Dead zone in ADC measurment at end stops

struct pedal {
  uint32_t raw = 0;
  uint32_t limit_low;
  uint32_t limit_high;
  uint32_t cc_value = 0;
  uint8_t pin;
  uint8_t cc;
};

pedal pedals[2];
USBMIDI usb_midi;
USBCompositeSerial usb_serial;

void setup() {
  USBComposite.clear();
  USBComposite.setProductId(0x0067);
  USBComposite.setVendorId(0x1eaa);
  USBComposite.setManufacturerString("riban");
  USBComposite.setProductString("Foot pedal x2");
  usb_midi.registerComponent();
  usb_serial.registerComponent();
  USBComposite.begin();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pedals[0].pin = PA0;
  pedals[0].cc = 67;
  pedals[1].pin = PA1;
  pedals[1].cc = 64;
  // Set inital ADC range, assuming pedals are at rest during power-up
  for (uint8_t i = 0; i < 2; ++i) {
    pedals[i].limit_high = analogRead(pedals[i].pin) - DEADZONE;
    pedals[i].limit_low = pedals[i].limit_high - DEADZONE;
  }
}

void loop() {
  static uint32_t flash_count = 0; // Counter used when flashing LED indicating MIDI traffic
  static uint32_t next_sec = 0; // Used for 1s events (mostly debug)
  static char msg[256]; // This framework does not support printf so use a buffer + sprintf + print

  // Process pedal ADC values
  for (uint32_t i = 0; i < 2; ++i){
    pedals[i].raw = analogRead(pedals[i].pin);
    // Auto-calibration of pedal range
    if (pedals[i].raw + DEADZONE < pedals[i].limit_low)
      pedals[i].limit_low = pedals[i].raw + DEADZONE;
    if (pedals[i].raw - DEADZONE > pedals[i].limit_high)
      pedals[i].limit_high = pedals[i].raw - DEADZONE;
    if (pedals[i].raw < pedals[i].limit_low) {
      // Endstop
      if (pedals[i].cc_value < 127) {
        pedals[i].cc_value = 127;
        usb_midi.sendControlChange(0, pedals[i].cc, 127);
      }
    } else if (pedals[i].raw > pedals[i].limit_high) {
      // Fully released
      if (pedals[i].cc_value > 0) {
        pedals[i].cc_value = 0;
        usb_midi.sendControlChange(0, pedals[i].cc, 0);
      }
    } else {
      uint32_t cc_value = 127 - (127 * (pedals[i].raw - pedals[i].limit_low) / (pedals[i].limit_high - pedals[i].limit_low - 1));
      uint32_t value = (EMA_A * (cc_value >> ADC_BITS_TO_IGNORE)) + ((1.0f - EMA_A) * pedals[i].cc_value);
      if (value != pedals[i].cc_value) {
        pedals[i].cc_value = value;
        usb_midi.sendControlChange(0, pedals[i].cc, value);
        flash_count = 100;
        digitalWrite(LED, LOW);
      }
    }
  }

  // Turn off LED if no recent MIDI message
  if (flash_count)
    if (--flash_count == 0)
      digitalWrite(LED, HIGH);

  delay(1); // May be worth adding some delay to avoid too rapid processing - allows ADCs to settle

  //Debug
  uint32_t now = millis();
  if (now > next_sec) {
    sprintf(msg, "SOFT: %03lu (%04lu:%04lu-%04lu) SUSTAIN: %03lu (%04lu:%04lu-%04lu)\n",
      pedals[0].cc_value, pedals[0].raw, pedals[0].limit_low, pedals[0].limit_high,
      pedals[1].cc_value, pedals[1].raw, pedals[1].limit_low, pedals[1].limit_high);
    usb_serial.print(msg);
    next_sec = now + 1000;
  }
}
