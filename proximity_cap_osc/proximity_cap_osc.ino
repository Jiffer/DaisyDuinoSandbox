/* Combines Adafruit MPR121 and VL53L0X examples
 *  with a DaisySP oscillator and filter
 *  
 *  assumes both are connected to I2C1 (pins 11, 12)
 *  An array of note values determines pitches
 *  the proximity sensor controls a filter cutoff frequency
 */
 #include "Adafruit_VL53L0X.h"
#include "Adafruit_MPR121.h"
#include "DaisyDuino.h"

DaisyHardware hw;
size_t num_channels;
static Oscillator osc;
MoogLadder filter;

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// Capacitive touch sensor
Adafruit_MPR121 cap = Adafruit_MPR121();
// ToF proximity sensor
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

int myNotes[] = {45, 48, 52, 55, 57, 59, 60, 62, 63, 64, 65, 67};
int note = 0;

void MyAudioCallback(float **in, float **out, size_t size) {
  // Convert Pitchknob MIDI Note Number to frequency
  osc.SetFreq(mtof(myNotes[note]));
  for (size_t i = 0; i < size; i++) {
    float sig = osc.Process();
    float filteredSig = filter.Process(sig);

    for (size_t chn = 0; chn < num_channels; chn++) {
      out[chn][i] = filteredSig;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");   
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  // Set Up Daisy Audio
  float sample_rate;
  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  sample_rate = DAISY.get_samplerate();

  // oscillator
  osc.Init(sample_rate);
  osc.SetFreq(440);
  osc.SetAmp(0.5);
  osc.SetWaveform(osc.WAVE_SAW);

  // initialize filter
  filter.Init(sample_rate);
  filter.SetRes(0.9); // expects value 0 - 1
  filter.SetFreq(1000);

  DAISY.begin(MyAudioCallback);
}


void loop() {
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    int measurement = measure.RangeMilliMeter;
    int cutoffFreq = measurement * 4;
    Serial.print("Distance (mm): "); Serial.println(measurement);
    Serial.print("setting filter freq: "); Serial.println(cutoffFreq);
    
    filter.SetFreq(cutoffFreq);
  } else {
    Serial.println(" out of range ");
  }

  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      // Serial.print(i); Serial.println(" touched");
      note = i;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      // Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;

  // here the delay causes a pause before the 
  // cap sense value and proximity update the note
  // and filter frequency 
  delay(100);
}
