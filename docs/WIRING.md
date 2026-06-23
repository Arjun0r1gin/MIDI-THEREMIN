# Wiring Guide — MIDI Theremin Controller

## Component Connections

### HC-SR04 Ultrasonic Sensors

**Note Sensor (Right Hand — controls pitch)**

| Sensor Pin | Arduino Pin |
|---|---|
| VCC | 5V |
| GND | GND |
| TRIG | D8 |
| ECHO | D9 |

**Volume Sensor (Left Hand — controls volume/pitch bend)**

| Sensor Pin | Arduino Pin |
|---|---|
| VCC | 5V |
| GND | GND |
| TRIG | D10 |
| ECHO | D11 |

---

### Push Buttons (all use INPUT_PULLUP — connect between pin and GND)

| Button | Arduino Pin | Function |
|---|---|---|
| BTN 0 | D2 | Scale ◄ |
| BTN 1 | D3 | Scale ► |
| BTN 2 | D4 | Octave ▼ |
| BTN 3 | D5 | Octave ▲ |
| BTN 4 | D6 | Instrument ◄ |
| BTN 5 | D7 | Instrument ► |

---

### SSD1306 OLED Display (I2C, 128×64)

| Display Pin | Arduino Pin |
|---|---|
| VCC | 3.3V or 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

---

## Notes

- All GND pins share a common ground rail on the breadboard.
- The OLED uses the hardware I2C bus (A4/A5) — do not use these pins for anything else.
- Buttons are wired with internal pull-ups (`INPUT_PULLUP`): one leg to the Arduino digital pin, the other to GND. No external resistor needed.
- Keep sensor wires away from the power supply lines to reduce noise in ultrasonic readings.
