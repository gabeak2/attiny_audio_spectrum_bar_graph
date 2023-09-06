#include "arduinoFFT.h"
#include <TM1640.h>
#include <TM16xxMatrix.h>
//#include <limits.h>
#include <float.h>

#define MIC 8
#define AVENUM 1
#define SCL 6
#define SDA 7

arduinoFFT FFT = arduinoFFT(); // Create FFT object 
TM1640 module(SDA, SCL);    // 
#define MATRIX_NUMCOLUMNS 16
#define MATRIX_NUMROWS 8
TM16xxMatrix matrix(&module, MATRIX_NUMCOLUMNS, MATRIX_NUMROWS);    // TM16xx object, columns, rows

const uint16_t samples = 64; //This value MUST ALWAYS be a power of 2

double vReal[samples];
double vImag[samples] = {0};

int limits[9] = {1,2,4,8,16,32,64,128,10000}; //Limits to determine height of a column based on FFT val
int bin_edges[9] = {0,1,3,6,10,15,21,28,32};

uint16_t getAnalogValue();

void setup()
{
  for(int i = 0; i < 8; i++){
    limits[i] = (limits[i])*8;
  }
//  analogSampleDuration(31);
  

  ADC0.CTRLA=ADC_ENABLE_bm|ADC_FREERUN_bm; // enable ADC and set free-running - need to check RESRDY to get new conversions
  ADC0.CTRLB = 0x00; // 64 samples accumulated  --> neeed to left shift 6 to get same adc results as without accumulating
  ADC0.CTRLC = 0x00; // 
  ADC0.CTRLD = 0x00; // 
  ADC0.SAMPCTRL = 0x00; // low impedance from op amp -> don't need to sample long
  ADC0.MUXPOS = 0x01; //AIN1 is the pin that gets op amp audio
  ADC0.COMMAND = 0x01; //start conversion
  //ADC0.INTCTRL = 0x01; //result ready interrupt
  
                    
  pinMode(MIC,INPUT);
  matrix.setAll(true);
  delay(200);
  module.setupDisplay(true, 4); // 2nd term is brightness - max==7
  delay(400);
  matrix.setAll(false);    // Note: module.clearDisplay() doesn't clear the offscreen bitmap!

  Serial.begin(9600);
 }


void loop(){

  double vImag[samples] = {0}; //always {0}
  double sum[samples] = {0};
    
  for(int i=0; i<samples; i++){
    vReal[i] = (double)(getAnalogValue()) ;
  }

  for(int i=0; i<samples; i++){
    Serial.println(vReal[i]);
  }
  
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */

//  int j = 2;
//  vReal[0] = sum[1]; //ignore 0th term - it's problematic with DC stuff
//  for(int i = 1; i < 16; i++){
//    vReal[i] = ((sum[j]+sum[j+1])/4); //summing multiple ffts 
//    j+= 2;
//  }

//  // klooge fudge factors
//  vReal[0] /=8;

  //{1,2,4,8,16,32,64,128,10000} LUT for reference
  for(int i = 0; i < 16; i++){ //determining what level each column should be based on LUT
    for(int j = 0; j <= 8; j++){

      if(vReal[i] < limits[j]){
        vReal[i] = j;
        break;
      }
      else if(j == 8){
        vReal[i] = 8;
        break;
      }
    }
  }
  matrix.setAll(true);
//  delay(1000);
  for(int i = 0; i < 16; i++){ //writing each column to proper level
    fill_col(i,vReal[i]); 
//    delay(1000);
  }

/* 
 *  Just a test to make sure the matrix display is working 

for(int j = 0; j < 16; j++){
  for(int i = 0;i<9;i++){
    fill_col(j,i);
    delay(50);
  }
}
*/
}

uint16_t getAnalogValue() {
 while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) { // Check for the RESRDY flag (Result Ready)
   ;//spin if there's no new result
 }
 return ADC0.RES; // return the current result on channel A - this clears the RESRDY flag
}

void fill_col(int col, int lvl){ //values written upside down by default
  
  int lut[9] = {0,128,192,224,240,248,252,254,255}; //LUT to fill columns
  
  if(lvl > 8){
    lvl = 8;
  }
  
  matrix.setColumn(col, lut[lvl]);
}
