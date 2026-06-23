#include <U8g2lib.h>
#include <Wire.h>
#include <avr/pgmspace.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C d(U8G2_R0, U8X8_PIN_NONE);

// ─── Pin Definitions ────────────────────────────────────────────────────────
const byte buttons[]  = {2, 3, 4, 5, 6, 7};
const byte trigNote   = 8,  echoNote = 9;
const byte trigVol    = 10, echoVol  = 11;

// ─── State Variables ─────────────────────────────────────────────────────────
byte currentScale      = 0;
byte currentOctave     = 4;
byte currentInstrument = 0;
byte currentChannel    = 0;
const byte rootNote    = 60;   // Middle C
int  lastNote          = -1;
int  lastChannel       = -1;
bool buttonStates[6]   = {};

// ─── Scales (stored in Flash) ────────────────────────────────────────────────
const byte sc0[] PROGMEM = {0, 2, 4, 5, 7, 9, 11};   // Ionian (Major)
const byte sc1[] PROGMEM = {0, 2, 3, 5, 7, 8, 11};   // Harmonic Minor
const byte sc2[] PROGMEM = {0, 2, 4, 7, 9};           // Pentatonic
const byte sc3[] PROGMEM = {0, 2, 4, 6, 8, 10};       // Whole Tone

const byte* const scales[]    PROGMEM = {sc0, sc1, sc2, sc3};
const byte        scaleLengths[] PROGMEM = {7, 7, 5, 6};

// ─── Name Tables (Flash) ─────────────────────────────────────────────────────
const char noteNames[12][3] PROGMEM = {
  "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

const char scaleNames[4][7] PROGMEM = {
  "Ion", "H.Min", "Penta", "Whole"
};

const char instrumentNames[16][10] PROGMEM = {
  "Piano",    "EPiano",   "Violin",   "Guitar",
  "VibeMstr", "Guzhen",   "Erhu",     "Koto",
  "BandDi",   "Percussn", "Trumpet",  "Flute",
  "Harmonic", "Crotalin", "Xylophon", "Drums"
};

// ─── Drum Note Maps ───────────────────────────────────────────────────────────
const byte drumRight[] = {36, 38, 50, 47, 43};   // Kick, Snare, Hi-Tom, Mid-Tom, Low-Tom
const byte drumLeft[]  = {42, 49, 51};            // Closed HH, Crash, Ride

// ─── Gesture Sensitivity ─────────────────────────────────────────────────────
const int NOTE_ZONE_SIZE = 5;    // cm per note zone
const int MIN_DISTANCE   = 3;    // cm minimum sensing distance

// ─── Pre-loaded Song: Janam Janam (Bm) — Channel 16 ─────────────────────────
const byte janamNotes[]     = {71,71,69,67,66,66,64,62,62,64,66,67,69,69,67,66};
const byte janamDurations[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
const byte janamLength      = sizeof(janamNotes);

// ─── Pre-loaded Song: Custom Ascending Melody — Channel 17 ───────────────────
const byte melodyNotes[]     = {72, 74, 76, 77, 79, 81, 83, 84};
const byte melodyDurations[] = {8,  8,  8,  8,  8,  8,  8,  8};
const byte melodyLength      = sizeof(melodyNotes);

byte          songIndex      = 0;
unsigned long lastNoteTime   = 0;
const int     noteDurationMs = 300;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  d.begin();

  for (byte i = 0; i < 6; i++) pinMode(buttons[i], INPUT_PULLUP);
  pinMode(trigNote, OUTPUT); pinMode(echoNote, INPUT);
  pinMode(trigVol,  OUTPUT); pinMode(echoVol,  INPUT);

  Serial.println("MIDI Theremin Ready");
}

// ─── Ultrasonic Distance Helper ───────────────────────────────────────────────
long getDistance(byte trigPin, byte echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH, 25000) / 58;
}

// ─── MIDI Helpers ─────────────────────────────────────────────────────────────
void sendMIDI(byte cmd, byte d1, byte d2 = 0) {
  Serial.write(cmd | currentChannel);
  Serial.write(d1);
  Serial.write(d2);
}

void sendNoteOff(byte channel, byte note) {
  Serial.write(0x80 | channel);
  Serial.write(note);
  Serial.write(0);
}

void sendPitchBend(int bend) {
  bend      = constrain(bend, 0, 16383);
  byte lsb  = bend & 0x7F;
  byte msb  = (bend >> 7) & 0x7F;
  Serial.write(0xE0 | currentChannel);
  Serial.write(lsb);
  Serial.write(msb);
}

void sendVolume(byte vol) {
  vol = constrain(vol, 0, 127);
  Serial.write(0xB0 | currentChannel);
  Serial.write(7);
  Serial.write(vol);
}

// ─── OLED Display ────────────────────────────────────────────────────────────
void printPGMStr(const char* p, int x, int y) {
  char buf[10];
  strcpy_P(buf, p);
  d.setCursor(x, y);
  d.print(buf);
}

void updateDisplay(const char* label, int note, long d1, long d2, int val) {
  d.firstPage();
  do {
    d.setFont(u8g2_font_5x7_tr);

    // Row 1 — Scale & Octave
    printPGMStr(scaleNames[currentScale], 2, 10);
    d.setCursor(40, 10); d.print("Oct:"); d.print(currentOctave);

    // Row 2 — Instrument & Channel
    d.setCursor(2, 20); d.print("Inst:");
    if      (currentChannel < 16) { printPGMStr(instrumentNames[currentChannel], 40, 20); }
    else if (currentChannel == 16) { d.setCursor(40, 20); d.print("Janam"); }
    else                            { d.setCursor(40, 20); d.print("Melody"); }
    d.setCursor(95, 20); d.print("Ch:"); d.print(currentChannel + 1);

    // Row 3 — Note
    d.setCursor(2, 30); d.print("Note:");
    if (note >= 0 && note < 128) {
      char noteBuf[4]; strcpy_P(noteBuf, noteNames[note % 12]);
      d.setCursor(40, 30); d.print(noteBuf); d.print(note / 12);
    }

    // Row 4-5 — Distances & Value
    d.setCursor(2, 40); d.print("Dist:"); d.print(d1); d.print("cm");
    d.setCursor(2, 50); d.print(label);   d.print(":"); d.print(val);
    d.drawFrame(0, 0, 128, 64);
  } while (d.nextPage());
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  // ── Button Handling ────────────────────────────────────────────────────────
  for (byte i = 0; i < 6; i++) {
    bool pressed = !digitalRead(buttons[i]);
    if (pressed && !buttonStates[i]) {
      switch (i) {
        case 0: currentScale      = (currentScale + 3) % 4; break;  // Scale ◄
        case 1: currentScale      = (currentScale + 1) % 4; break;  // Scale ►
        case 2: if (currentOctave > 1) currentOctave--;      break;  // Oct  ▼
        case 3: if (currentOctave < 7) currentOctave++;      break;  // Oct  ▲
        case 4:                                                        // Inst ◄
          if (currentInstrument > 0)  { currentInstrument--; currentChannel = currentInstrument; songIndex = 0; }
          break;
        case 5:                                                        // Inst ►
          if (currentInstrument < 17) { currentInstrument++; currentChannel = currentInstrument; songIndex = 0; }
          break;
      }
    }
    buttonStates[i] = pressed;
  }

  long noteDist = getDistance(trigNote, echoNote);
  long volDist  = getDistance(trigVol,  echoVol);

  // ── Drum Mode (Ch 15 / MIDI ch 10) ────────────────────────────────────────
  if (currentChannel == 15) {
    if (noteDist >= 3 && noteDist <= 30) {
      byte idx  = constrain(map(noteDist, 3, 30, 0, sizeof(drumRight) - 1), 0, sizeof(drumRight) - 1);
      sendMIDI(0x99, drumRight[idx], 100);
    }
    if (volDist >= 3 && volDist <= 30) {
      byte idx  = constrain(map(volDist,  3, 30, 0, sizeof(drumLeft)  - 1), 0, sizeof(drumLeft)  - 1);
      sendMIDI(0x99, drumLeft[idx],  100);
    }
    updateDisplay("Drum", -1, noteDist, volDist, -1);
    delay(150);
    return;
  }

  // ── Janam Janam Playback (Ch 16) ──────────────────────────────────────────
  if (currentChannel == 16) {
    if (millis() - lastNoteTime > noteDurationMs) {
      sendNoteOff(16, janamNotes[songIndex]);
      sendMIDI(0x90 | 16, janamNotes[songIndex], 100);
      lastNoteTime = millis();
      songIndex    = (songIndex + 1) % janamLength;
    }
    long vol = map(volDist, 3, 30, 127, 0);
    sendVolume(vol);
    updateDisplay("Janam", janamNotes[songIndex], noteDist, volDist, vol);
    delay(20);
    return;
  }

  // ── Custom Melody Playback (Ch 17) ────────────────────────────────────────
  if (currentChannel == 17) {
    if (millis() - lastNoteTime > noteDurationMs) {
      sendNoteOff(17, melodyNotes[songIndex]);
      sendMIDI(0x90 | 17, melodyNotes[songIndex], 100);
      lastNoteTime = millis();
      songIndex    = (songIndex + 1) % melodyLength;
    }
    long vol = map(volDist, 3, 30, 127, 0);
    sendVolume(vol);
    updateDisplay("Melody", melodyNotes[songIndex], noteDist, volDist, vol);
    delay(20);
    return;
  }

  // ── Normal Theremin Note Mode ─────────────────────────────────────────────
  byte len         = pgm_read_byte(&scaleLengths[currentScale]);
  int  maxDist     = MIN_DISTANCE + (NOTE_ZONE_SIZE * len);
  byte si          = constrain(map(noteDist, MIN_DISTANCE, maxDist, 0, len - 1), 0, len - 1);
  byte step        = pgm_read_byte((const byte*)pgm_read_ptr(&scales[currentScale]) + si);
  int  currentNote = rootNote + step + (currentOctave - 4) * 12;

  if (currentNote != lastNote || currentChannel != lastChannel) {
    if (lastNote >= 0 && lastChannel >= 0) sendNoteOff(lastChannel, lastNote);
    sendMIDI(0x90, currentNote, 127);
    lastNote    = currentNote;
    lastChannel = currentChannel;
  }

  // Pitch-bend enabled channels: Piano, EPiano, Harmonic
  if (currentChannel == 0 || currentChannel == 1 || currentChannel == 12) {
    sendVolume(127);
    int bend = map(volDist, 3, 30, 0, 16383);
    sendPitchBend(bend);
    updateDisplay("PBend", currentNote, noteDist, volDist, bend);
  } else {
    byte vol = map(volDist, 3, 30, 127, 0);
    sendVolume(vol);
    updateDisplay("Vol", currentNote, noteDist, volDist, vol);
  }
  delay(20);
}
