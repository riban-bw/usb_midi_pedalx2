#include <Arduino.h>

#include <USBComposite.h>

#define EMA_A 0.2f // filter coeficient (0..1 higher value gives less agressive filter - 1.0 would pass all data, unfiltered)
#define ADC_BITS_TO_IGNORE 0 // Can reduce resolution if too noisey
#define LIMIT_LOW 1300
#define LIMIT_HIGH 1700
#define LED PC13
#define SUSTAIN_ADC PA1
#define SOFT_ADC PA0

struct pedal {
  uint32_t raw = 0;
  uint32_t limit_low = LIMIT_LOW;
  uint32_t limit_high = LIMIT_HIGH;
  uint32_t last_cc_value = 0;
  uint32_t velocity = 0;
  uint8_t triggered = 0;
  uint8_t pin;
  uint8_t cc;
};

uint32_t last_value = 0;
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
  pedals[0].pin = SOFT_ADC;
  pedals[0].cc = 67;
  pedals[1].pin = SUSTAIN_ADC;
  pedals[1].cc = 64;
  for (uint8_t i = 0; i < 2; ++i) {
    pedals[i].limit_high = analogRead(pedals[i].pin) - 100;
    pedals[i].limit_low = pedals[i].limit_high - 100;
  }
}

void loop() {
  static uint32_t flash_count = 0;
  static uint32_t next_sec = 0;
  static char msg[256];

  for (uint32_t i = 0; i < 2; ++i){
    pedals[i].raw = analogRead(pedals[i].pin);
    if (pedals[i].raw + 100 < pedals[i].limit_low)
      pedals[i].limit_low = pedals[i].raw + 100;
    if (pedals[i].raw - 100 > pedals[i].limit_high)
      pedals[i].limit_high = pedals[i].raw - 100;
    if (pedals[i].raw < pedals[i].limit_low) {
      if (pedals[i].last_cc_value < 127) {
        pedals[i].last_cc_value = 127;
        pedals[i].velocity = 0;
        pedals[i].triggered = 0;
        usb_midi.sendControlChange(0, pedals[i].cc, 127);
      }
    } else if (pedals[i].raw > pedals[i].limit_high) {
      if (pedals[i].last_cc_value > 0) {
        pedals[i].last_cc_value = 0;
        usb_midi.sendControlChange(0, pedals[i].cc, 0);
      }
    } else {
      uint32_t cc_value = 127 - (127 * (pedals[i].raw - pedals[i].limit_low) / (pedals[i].limit_high - pedals[i].limit_low - 1));
      uint32_t value = (EMA_A * (cc_value >> ADC_BITS_TO_IGNORE)) + ((1.0f - EMA_A) * pedals[i].last_cc_value);
      if (value != pedals[i].last_cc_value) {
        pedals[i].last_cc_value = value;
        usb_midi.sendControlChange(0, pedals[i].cc, value);
        flash_count = 100;
        digitalWrite(LED, LOW);
      }
      if (pedals[i].velocity == 0)
        pedals[i].velocity = millis();
      else if (pedals[i].triggered == 0 && cc_value > 10) {
        pedals[i].triggered = 1;
      }
    }
  }
  if (flash_count)
    if (--flash_count == 0)
      digitalWrite(LED, HIGH);
  delay(1);

  //Debug
  uint32_t now = millis();
  if (now > next_sec) {
    sprintf(msg, "SOFT: %03lu (%04lu:%04lu-%04lu) SUSTAIN: %03lu (%04lu:%04lu-%04lu)\n",
      pedals[0].last_cc_value, pedals[0].raw, pedals[0].limit_low, pedals[0].limit_high,
      pedals[1].last_cc_value, pedals[1].raw, pedals[1].limit_low, pedals[1].limit_high);
    usb_serial.print(msg);
    next_sec = now + 1000;
  }
}
