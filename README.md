# 🎵 MIDI Theremin Controller

> A gesture-controlled MIDI instrument built with Arduino Uno and ultrasonic sensors — play music through the air, without touching anything.

---

## 📖 Overview

The **MIDI Theremin Controller** is an Arduino-based musical instrument that uses two HC-SR04 ultrasonic sensors to detect hand position and translate it into MIDI messages in real time. One hand controls the **note/pitch**, the other controls **volume or pitch bend** — just like a classic theremin, but outputting digital MIDI to any DAW or synthesizer.

Built as a mini-project for the **2nd Semester B.E. (CSE)** at East Point College of Engineering & Technology, Bangalore (2024–25).

---

## ✨ Features

| Feature | Details |
|---|---|
| 🎼 Scale Selection | Ionian, Harmonic Minor, Pentatonic, Whole Tone |
| 🎹 18 Instruments | Piano, Violin, Erhu, Drums, Koto, and more |
| 🥁 Drum Mode | Gesture-triggered drum hits on MIDI Ch 10 |
| 🎵 Song Playback | Pre-loaded melodies (Janam Janam + custom scale) |
| 🎛️ Pitch Bend | Left-hand distance bends pitch on supported channels |
| 📟 OLED Display | Real-time feedback: note, scale, octave, channel |
| 🔘 6 Push Buttons | Switch scale, octave, and instrument on-the-fly |

---

## 🔧 Hardware Components

| Component | Qty | Purpose |
|---|---|---|
| Arduino Uno (ATmega328P) | 1 | Main microcontroller |
| HC-SR04 Ultrasonic Sensor | 2 | Note & volume sensing |
| SSD1306 OLED Display (128×64) | 1 | Real-time feedback |
| Tactile Push Buttons | 6 | Scale / octave / instrument control |
| Breadboard | 1 | Prototyping |
| Jumper Wires | — | Connections |
| USB Cable | 1 | Power + MIDI data |

---

## 📌 Pin Configuration

```
Buttons   →  D2, D3, D4, D5, D6, D7
Note Sensor:  TRIG → D8   |  ECHO → D9
Vol Sensor:   TRIG → D10  |  ECHO → D11
OLED:         SDA → A4    |  SCL  → A5  (I2C)
```

---

## 🎮 Button Controls

| Button | Pin | Action |
|---|---|---|
| BTN 0 | D2 | Scale ◄ (cycle back) |
| BTN 1 | D3 | Scale ► (cycle forward) |
| BTN 2 | D4 | Octave ▼ (down, min 1) |
| BTN 3 | D5 | Octave ▲ (up, max 7) |
| BTN 4 | D6 | Instrument ◄ (previous) |
| BTN 5 | D7 | Instrument ► (next) |

---

## 🎵 How It Works

```
Right Hand (Note Sensor)          Left Hand (Volume Sensor)
         │                                   │
 Distance → Scale Step               Distance → Volume / Pitch Bend
         │                                   │
         └────────────── Arduino Uno ─────────┘
                              │
                         MIDI over USB
                              │
                    DAW / Synthesizer / MIDI Device
```

1. **Right hand** distance is mapped to a note in the selected scale.
2. **Left hand** distance controls volume (or pitch bend for Piano/EPiano/Harmonic channels).
3. **Buttons** switch scale, octave, and instrument in real time.
4. An **OLED display** shows the current note, scale, octave, channel, and sensor readings.

---

## 🎼 Available Scales

| ID | Scale | Notes |
|---|---|---|
| 0 | Ionian (Major) | 0–2–4–5–7–9–11 |
| 1 | Harmonic Minor | 0–2–3–5–7–8–11 |
| 2 | Pentatonic | 0–2–4–7–9 |
| 3 | Whole Tone | 0–2–4–6–8–10 |

---

## 🎹 Instrument / Channel Map

| Ch | Instrument | Pitch Bend? |
|---|---|---|
| 1 | Piano | ✅ |
| 2 | EPiano | ✅ |
| 3 | Violin | — |
| 4 | Guitar | — |
| 5–14 | Various (Koto, Erhu, Flute…) | — |
| 13 | Harmonic | ✅ |
| 15 | Drums (MIDI Ch 10) | Gesture drums |
| 16 | Janam Janam Playback | Auto-play |
| 17 | Custom Melody Playback | Auto-play |

---

## 💻 Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- **Libraries**: `U8g2` (OLED), `Wire` (I2C)
- [Hairless MIDI](https://projectgus.github.io/hairless-midiserial/) — Serial → MIDI bridge
- [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) — Virtual MIDI port
- Any DAW: FL Studio, Ableton Live, Cakewalk, GarageBand, etc.

### Installing Libraries (Arduino IDE)

```
Sketch → Include Library → Manage Libraries
  → Search "U8g2" → Install
```

---

## 🚀 Getting Started

1. **Wire** the components as per the pin configuration above.
2. **Install** the required Arduino libraries.
3. **Upload** `src/midi_theremin.ino` to the Arduino Uno.
4. **Open** Hairless MIDI → set serial port to Arduino's COM port → baud `115200`.
5. **Open** loopMIDI → create a virtual port.
6. **Connect** Hairless MIDI output to the loopMIDI port.
7. **Open your DAW** → select the loopMIDI port as MIDI input.
8. **Play** — move your right hand to change notes, left hand for volume!

---

## ⚡ Sensing Range & Sensitivity

| Parameter | Value |
|---|---|
| Minimum sensing distance | 3 cm |
| Note zone size | 5 cm per scale step |
| Drum trigger range | 3–30 cm |
| Volume/pitch bend range | 3–30 cm |
| Update rate | ~50 Hz (20 ms loop) |
| Drum mode update rate | ~7 Hz (150 ms) |

---

## 📊 Project Architecture

```
midi-theremin/
├── src/
│   └── midi_theremin.ino    # Main Arduino sketch
├── docs/
│   └── project_report.pdf   # Full academic report
├── diagrams/
│   └── block_diagram.png    # System block diagram
├── media/
│   └── (project photos)
└── README.md
```

---

## ✅ Advantages

- **Low cost** — standard off-the-shelf Arduino components
- **Hands-free** — gesture-based, no physical keys required
- **Multi-modal** — 18 instruments, 4 scales, 7 octaves
- **Expandable** — easy to add knobs, sliders, or wireless MIDI

## ⚠️ Known Limitations

- Ultrasonic sensors can fluctuate near reflective surfaces
- Requires external software (Hairless MIDI + loopMIDI) on PC
- No polyphony — one note at a time per sensor axis

---

## 🔭 Future Scope

- [ ] Bluetooth / USB-MIDI for wireless operation
- [ ] Velocity-sensitive input
- [ ] Touch or capacitive strip for continuous pitch control
- [ ] Preset memory (save favourite scale/instrument combos)
- [ ] Mobile app integration

---

## 👥 Authors

| Name | ID |
|---|---|
| Aadityaraaj Pandit | 1EP24IC001 |
| Arjun V | 1EP24IC007 |
| Himanshu Kumar | 1EP24IC014 |
| Vagish P. Shanbhag | 1EP24IC055 |

**Guide:** Mrs. Sandhya N, Assistant Professor, Dept. of ECE, EPCET

---

## 📚 References

1. [Nerd Musician — Arduino MIDI Tutorial](https://www.youtube.com/@NerdMusician)
2. Wessel & Wright (2002) — *Problems and Prospects for Intimate Musical Control of Computers*, NIME
3. Sahoo et al. (2020) — *Design and Implementation of a MIDI Controller Using Arduino*, IJERT
4. Kapur et al. (2004) — *Mapping Strategies for Musical Performance*, NIME
5. Kolli & Teja (2018) — *MIDI Controller for Virtual Instruments using Arduino Uno*, IEEE ICCIC

---

## 📄 License

This project is open-source under the [MIT License](LICENSE).
