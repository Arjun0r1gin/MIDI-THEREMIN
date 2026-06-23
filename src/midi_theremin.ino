#include <U8g2lib.h>
#include <Wire.h>
#include <avr/pgmspace.h>

U8G2_SSD1306_128X64_NONAME_1_HW_I2C d(U8G2_R0, U8X8_PIN_NONE);

// Pins
const byte buttons[] = {2, 3, 4, 5, 6, 7};
const byte trigNote = 8, echoNote = 9;
const byte trigVol = 10, echoVol = 11;

// States
byte currentScale = 0, currentOctave = 4, currentInstrument = 0;
byte currentChannel = 0;
const byte rootNote = 60;
int lastNote = -1;
int lastChannel = -1;
int lastDrumIdxR = -1;
int lastDrumIdxL = -1;
bool buttonStates[6] = {};

// Scales
const byte sc0[] PROGMEM = {0, 2, 4, 5, 7, 9, 11};
const byte sc1[] PROGMEM = {0, 2, 3, 5, 7, 8, 11};
const byte sc2[] PROGMEM = {0, 2, 4, 7, 9};
const byte sc3[] PROGMEM = {0, 2, 4, 6, 8, 10};
const byte* const scales[] PROGMEM = {sc0, sc1, sc2, sc3};
const byte scaleLengths[] PROGMEM = {7, 7, 5, 6};

const char noteNames[12][3] PROGMEM = {
  "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

const char scaleNames[4][7] PROGMEM = {
  "Ion", "H.Min", "Penta", "Whole"
};

const char instrumentNames[16][10] PROGMEM = {
  "Piano", "EPiano", "Violin", "Guitar", "VibeMstr", "Guzhen", "Erhu", "Koto",
  "BandDi", "Percussn", "Trumpet", "Flute", "Harmonic", "Crotalin", "Xylophon", "Drums"
};

//  Drum mapping (updated to your requested instruments & order) 
// Left sensor (volDist): Snare(0-5), High Tom(6-10), Hi-Hat(11-15), last slot unused (silent)
const byte drumLeft[]  = {38, 50, 42, 0}; // 38=Snare,50=HighTom,42=Closed Hi-Hat, last silent

// Right sensor (noteDist): Bass(0-5), FloorTom(6-10), MidTom(11-15), Crash(16-20), last slot unused (silent)
const byte drumRight[] = {36, 41, 47, 49, 0}; // 36=Bass/Kick,41=FloorTom,47=MidTom,49=Crash, last silent

const uint8_t DRUM_RIGHT_COUNT = sizeof(drumRight) / sizeof(drumRight[0]);
const uint8_t DRUM_LEFT_COUNT  = sizeof(drumLeft)  / sizeof(drumLeft[0]);


const int NOTE_ZONE_SIZE = 5;
const int MIN_DISTANCE = 3;

// Janam Janam (Bm) - Channel 16
const byte janamNotes[] = {71,71,69,67,66,66,64,62,62,64,66,67,69,69,67,66};
const byte janamDurations[] = {8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
const byte janamLength = sizeof(janamNotes);

// Custom Melody - Channel 17
const byte melodyNotes[] = {72,74,76,77,79,81,83,84};
const byte melodyDurations[] = {8,8,8,8,8,8,8,8};
const byte melodyLength = sizeof(melodyNotes);

byte songIndex = 0;
unsigned long lastNoteTime = 0;
const int noteDurationMs = 300;

// Volume-filtering additions
const byte VOL_SAMPLE_COUNT = 5;       
const float VOL_SMOOTH_ALPHA = 0.28;   
const int VOL_FAR_CM = 30;             
float smoothedVolDist = VOL_FAR_CM;    

// Drum hit state (prevent auto-trigger)
unsigned long lastDrumRightHit = 0;
unsigned long lastDrumLeftHit  = 0;
int lastDrumVelR = 0;
int lastDrumVelL = 0;

const unsigned long DRUM_MIN_INTERVAL_MS = 120; // min time between hits per side


bool volHasEcho = false;
bool noteHasEcho = false;

long singleDistanceForVol(byte trigPin, byte echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long t = pulseIn(echoPin, HIGH, 30000);
  if (t == 0) return 0;
  return t / 58;
}

long getMedianDistanceVol(byte trigPin, byte echoPin) {
  long samples[VOL_SAMPLE_COUNT];
  for (byte i = 0; i < VOL_SAMPLE_COUNT; i++) {
    samples[i] = singleDistanceForVol(trigPin, echoPin);
    delay(5);
  }
  // insertion sort (small array)
  for (byte i = 1; i < VOL_SAMPLE_COUNT; i++) {
    long key = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }
  return samples[VOL_SAMPLE_COUNT / 2];
}

void setup() {
  Serial.begin(115200);
  d.begin();

  for (byte i = 0; i < 6; i++) pinMode(buttons[i], INPUT_PULLUP);
  pinMode(trigNote, OUTPUT); pinMode(echoNote, INPUT);
  pinMode(trigVol, OUTPUT); pinMode(echoVol, INPUT);

  Serial.println("MIDI Theremin Ready");
}

long getDistance(byte trigPin, byte echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH, 25000) / 58;
}

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
  bend = constrain(bend, 0, 16383);
  byte lsb = bend & 0x7F;
  byte msb = (bend >> 7) & 0x7F;
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

    printPGMStr(scaleNames[currentScale], 2, 10);
    d.setCursor(40, 10); d.print("Oct:"); d.print(currentOctave);

    d.setCursor(2, 20); d.print("Inst:");
    if (currentChannel < 16) {
      printPGMStr(instrumentNames[currentChannel], 40, 20);
    } else if (currentChannel == 16) {
      d.setCursor(40, 20); d.print("Janam");
    } else if (currentChannel == 17) {
      d.setCursor(40, 20); d.print("Melody");
    }

    d.setCursor(95, 20); d.print("Ch:"); d.print(currentChannel + 1);

    d.setCursor(2, 30); d.print("Note:");
    if (note >= 0 && note < 128) {
      char noteBuf[4]; strcpy_P(noteBuf, noteNames[note % 12]);
      d.setCursor(40, 30); d.print(noteBuf); d.print(note / 12);
    }

    d.setCursor(2, 40); d.print("Dist:"); d.print(d1); d.print("cm");
    d.setCursor(2, 50); d.print(label); d.print(":"); d.print(val);
    d.drawFrame(0, 0, 128, 64);
  } while (d.nextPage());
}

void loop() {
  for (byte i = 0; i < 6; i++) {
    bool pressed = !digitalRead(buttons[i]);
    if (pressed && !buttonStates[i]) {
      switch (i) {
        case 0: currentScale = (currentScale + 3) % 4; break;
        case 1: currentScale = (currentScale + 1) % 4; break;
        case 2: if (currentOctave > 1) currentOctave--; break;
        case 3: if (currentOctave < 7) currentOctave++; break;
        case 4:
          if (currentInstrument > 0) {
            currentInstrument--;
            currentChannel = currentInstrument;
            songIndex = 0;
          }
          break;
        case 5:
          if (currentInstrument < 17) {
            currentInstrument++;
            currentChannel = currentInstrument;
            songIndex = 0;
          }
          break;
      }
    }
    buttonStates[i] = pressed;
  }

 
  long noteRaw = getDistance(trigNote, echoNote);
  // if pulseIn returned 0, treat as no-echo
  if (noteRaw == 0) {
    noteHasEcho = false;
  } else {
    noteHasEcho = true;
  }
  long noteDist = noteRaw; // may be -1 substitute later

  // VOLUME sensor:
  long rawVol = getMedianDistanceVol(trigVol, echoVol);
  
  if (rawVol == 0) {
    volHasEcho = false;
  } else {
    volHasEcho = true;
   
    smoothedVolDist = VOL_SMOOTH_ALPHA * rawVol + (1.0 - VOL_SMOOTH_ALPHA) * smoothedVolDist;
    if (smoothedVolDist < MIN_DISTANCE) smoothedVolDist = MIN_DISTANCE;
    if (smoothedVolDist > VOL_FAR_CM) smoothedVolDist = VOL_FAR_CM;
  }

  
  long volDist;
  if (!volHasEcho) volDist = -1;
  else volDist = (long)round(smoothedVolDist);

  
  if (!noteHasEcho) noteDist = -1;

  
if (currentChannel == 15) {
  static int prevIdxR = -1;
  static int prevIdxL = -1;

  // RIGHT sensor (noteDist)
  if (noteDist >= 3 && noteDist <= 30) {
    // map into 0..(count-1)
    int drumIdxR = map((int)noteDist, 3, 30, 0, DRUM_RIGHT_COUNT - 1);
    drumIdxR = constrain(drumIdxR, 0, DRUM_RIGHT_COUNT - 1);

  
    if (drumIdxR != (int)DRUM_RIGHT_COUNT - 1) {
      // Only trigger when index changes (entering a new zone)
      if (drumIdxR != prevIdxR) {
        byte noteR = drumRight[drumIdxR];
        sendMIDI(0x99, noteR, 100);
        prevIdxR = drumIdxR;
      }
    } else {
      
      prevIdxR = -1;
    }
  } else {
    
    prevIdxR = -1;
  }

  // LEFT sensor (volDist)
  if (volDist >= 3 && volDist <= 30) {
    int drumIdxL = map((int)volDist, 3, 30, 0, DRUM_LEFT_COUNT - 1);
    drumIdxL = constrain(drumIdxL, 0, DRUM_LEFT_COUNT - 1);

    if (drumIdxL != (int)DRUM_LEFT_COUNT - 1) {
      if (drumIdxL != prevIdxL) {
        byte noteL = drumLeft[drumIdxL];
        sendMIDI(0x99, noteL, 100);
        prevIdxL = drumIdxL;
      }
    } else {
      prevIdxL = -1;
    }
  } else {
    prevIdxL = -1;
  }

  updateDisplay("Drum", -1, noteDist, volDist, -1);
  delay(150);
  return;
}
//  end drum branch 

  // end drum channel 
  // Janam Janam Song (Channel 16)
  if (currentChannel == 16) {
    if (millis() - lastNoteTime > noteDurationMs) {
      sendNoteOff(16, janamNotes[songIndex]);
      sendMIDI(0x90 | 16, janamNotes[songIndex], 100);
      lastNoteTime = millis();
      songIndex = (songIndex + 1) % janamLength;
    }
    long vol = volDist;
    if (vol == -1) vol = VOL_FAR_CM; // if no echo treat as far for volume song
    long volMapped = map(vol, MIN_DISTANCE, VOL_FAR_CM, 127, 0);
    volMapped = constrain(volMapped, 0, 127);
    sendVolume(volMapped);
    updateDisplay("Janam", janamNotes[songIndex], noteDist, volDist, volMapped);
    delay(20);
    return;
  }

  // Custom Melody (Channel 17)
  if (currentChannel == 17) {
    if (millis() - lastNoteTime > noteDurationMs) {
      sendNoteOff(17, melodyNotes[songIndex]);
      sendMIDI(0x90 | 17, melodyNotes[songIndex], 100);
      lastNoteTime = millis();
      songIndex = (songIndex + 1) % melodyLength;
    }
    long vol = volDist;
    if (vol == -1) vol = VOL_FAR_CM;
    long volMapped = map(vol, MIN_DISTANCE, VOL_FAR_CM, 127, 0);
    volMapped = constrain(volMapped, 0, 127);
    sendVolume(volMapped);
    updateDisplay("Melody", melodyNotes[songIndex], noteDist, volDist, volMapped);
    delay(20);
    return;
  }

  // Normal Note Mode
  byte len = pgm_read_byte(&scaleLengths[currentScale]);
  int maxDist = MIN_DISTANCE + (NOTE_ZONE_SIZE * len);
  byte si = constrain(map((noteDist == -1 ? VOL_FAR_CM : noteDist), MIN_DISTANCE, maxDist, 0, len - 1), 0, len - 1);
  byte step = pgm_read_byte((const byte*)pgm_read_ptr(&scales[currentScale]) + si);
  int currentNote = rootNote + step + (currentOctave - 4) * 12;

  if (currentNote != lastNote || currentChannel != lastChannel) {
    if (lastNote >= 0 && lastChannel >= 0) sendNoteOff(lastChannel, lastNote);
    sendMIDI(0x90, currentNote, 127);
    lastNote = currentNote;
    lastChannel = currentChannel;
  }

  if (currentChannel == 0 || currentChannel == 1 || currentChannel == 12) {
    sendVolume(127);
    int bend = (volDist == -1) ? 0 : map(volDist, MIN_DISTANCE, VOL_FAR_CM, 0, 16383);
    bend = constrain(bend, 0, 16383);
    sendPitchBend(bend);
    updateDisplay("PBend", currentNote, noteDist, volDist, bend);
  } else {
    byte volMapped = (volDist == -1) ? 0 : map(volDist, MIN_DISTANCE, VOL_FAR_CM, 127, 0);
    volMapped = constrain(volMapped, 0, 127);
    sendVolume(volMapped);
    updateDisplay("Vol", currentNote, noteDist, volDist, volMapped);
  }
  delay(20);
}
