/*Based on original code by juliaandwillett: https://create.arduino.cc/projecthub/juliandwillett/customizable-geiger-muller-counter-e847d1 */
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35 /*No buttons used yet*/
#define BUTTON_2            0
unsigned long counts; //variable for GM Tube events
unsigned long previousMillis; //variable for measuring time
float averageCPM;
float sdCPM;
int currentCPM;
float calcCPM;
float CPMArray[100];
int maxCPM;
int vref = 1100;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom TFT LCD library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
char buff[512];


#define LOG_PERIOD 1000 // count rate (in milliseconds)
void setup() {
  counts = 0;
  currentCPM = 0;
  averageCPM = 0;
  maxCPM = 0;
  sdCPM = 0;
  calcCPM = 0;
  Serial.begin(9600);
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), impulse, FALLING); //define external interrupts
  tft.init(); // Init TFT LCD
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
}

void loop() { //main count cycle
  delay(1000); //This prevents text overwrites as the count increases
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print("CPM count: ");
  tft.println(counts);
  tft.print("Max CPM: ");
  tft.println(maxCPM);
  Serial.println("CPM");
  Serial.print(counts);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > LOG_PERIOD) {
    previousMillis = currentMillis;
    CPMArray[currentCPM] = counts * 2;
    tft.setCursor(0, 32);
    tft.print("uSv/hr: ");
    tft.println(outputSieverts(CPMArray[currentCPM]));
    if (counts > maxCPM){
      maxCPM = counts;
      }
    counts = 0;
    averageCPM = 0;
    sdCPM = 0;
    //calc avg and sd
    for (int x = 0; x < currentCPM + 1; x++)  {
      averageCPM = averageCPM + CPMArray[x];
    }
    averageCPM = averageCPM / (currentCPM + 1);
    for (int x = 0; x < currentCPM + 1; x++)  {
      sdCPM = sdCPM + sq(CPMArray[x] - averageCPM);
    }
    sdCPM = sqrt(sdCPM / currentCPM) / sqrt(currentCPM + 1);

    Serial.print("Avg uSv: " + String(averageCPM) + " +/- " + String(sdCPM) + "  ArrayVal: " + String(CPMArray[currentCPM]));
    currentCPM = currentCPM + 1;
    displayAverageCPM();
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "V+ :" + String(battery_voltage) + "V";
        Serial.print(voltage);
        tft.setTextDatum(MC_DATUM);
        tft.println(voltage);
    
    }
  }
}
  void impulse() {
    counts++;
  }
  void displayAverageCPM()  {
    tft.setCursor(0, 64);
    tft.print("Avg: ");
    tft.print(outputSieverts(averageCPM));
    tft.print("+/-");
    tft.println(outputSieverts(sdCPM));
  }
  float outputSieverts(float x)  {
    float y = x * 0.0057;
    return y;
  }
    
  
