// Title: adsrOscAnalogRead
// Description: combines adsr and oscillator by Ben Sergentanis
// Triangle wave controlled by adsr envelope
// Analog input A0 controls pitch
// digital input D30 controls ADSR envelope
// Hardware: Daisy Seed
// Author: Jiffer Harriman

#include "DaisyDuino.h"

DaisyHardware hw;

size_t num_channels;

Adsr env;
Oscillator osc;
Metro tick;
bool gate;
bool noteOnOff = false;

// digital input pin that controls ADSR
int buttonPin = D30;
unsigned long printTimer;
unsigned int printTime = 500; // print every 500ms

float pitchknob;

void MyCallback(float **in, float **out, size_t size) {
  float osc_out, env_out;
  for (size_t i = 0; i < size; i++) {

    // When the metro ticks (bang)
    if (tick.Process()) {
      // Convert Pitchknob MIDI Note Number to frequency
      osc.SetFreq(mtof(pitchknob));
    }

    // Use envelope to control the amplitude of the oscillator.
    env_out = env.Process(noteOnOff);
    osc.SetAmp(env_out);
    osc_out = osc.Process();

    for (size_t chn = 0; chn < num_channels; chn++) {
      out[chn][i] = osc_out;
    }
  }
}

void setup() {
  float samplerate;
  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  samplerate = DAISY.get_samplerate();

  env.Init(samplerate);
  osc.Init(samplerate);

  // Set up metro to pulse 6 times / second
  // this is when it will update the oscillator frequency
  tick.Init(6.f, samplerate);

  // Set envelope parameters
  env.SetTime(ADSR_SEG_ATTACK, .01);
  env.SetTime(ADSR_SEG_DECAY, .1);
  env.SetTime(ADSR_SEG_RELEASE, .25);
  env.SetSustainLevel(.25);

  // Set parameters for oscillator
  osc.SetWaveform(osc.WAVE_TRI);

  // configure pin for digital read to control ADSR
  pinMode(buttonPin, INPUT_PULLUP);

  DAISY.begin(MyCallback);
  Serial.begin(115200);
}

void loop() {
  int a0Val = analogRead(A0);
  // map A0 into a number range for MIDI to Frequency (mtof)
  pitchknob = 36.0 + ((a0Val / 1023.0) * 48.0);
  bool dVal = digitalRead(buttonPin);
  noteOnOff = !dVal; // Active low button

  // this only runs at printTime interval (every 500 ms)
  if (millis() > printTimer) {
    printTimer = millis() + printTime;
    Serial.print("pitchknob calculation: ");
    Serial.println(a0Val);
    
    Serial.print("pin D30: ");
    Serial.println(dVal);
  }
}
