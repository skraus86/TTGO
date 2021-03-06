/*Based on original code by juliaandwillett: https://create.arduino.cc/projecthub/juliandwillett/customizable-geiger-muller-counter-e847d1 */


#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"
#include "Arduino.h"

#define M_SIZE 1.067 // Define meter size
#define TFT_GREY 0x5AEB
#define TFT_ORANGE      0xFD20      /* 255, 165,   0 */
#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35 /*No buttons used yet*/
#define BUTTON_2            0
#define LOG_PERIOD 1000 // count rate (in milliseconds)

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = M_SIZE*120, osy = M_SIZE*120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update
unsigned long counts; //variable for GM Tube events
unsigned long previousMillis; //variable for measuring time
float averageCPM;
float sdCPM;
int currentCPM;
int correctCPM;
int bufCounts;
float calcCPM;
float CPMArray[100];
int maxCPM;
int vref = 1100;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom TFT LCD library
//Button2 btn1(BUTTON_1);
Button2 buttonB = Button2(BUTTON_1);
char buff[512];

int old_analog =  -999; // Value last displayed

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;

void setup() {
  Serial.begin(9600); // For debug
  counts = 0;
  currentCPM = 0;
  averageCPM = 0;
  maxCPM = 0;
  sdCPM = 0;
  calcCPM = 0;
  correctCPM= 0;
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  pinMode(2, INPUT); //set pins for Geiger Signal
  attachInterrupt(digitalPinToInterrupt(2), impulse, FALLING); //define external interrupts
  buttonB.setLongClickHandler(longpress);
  analogMeter(); // Draw analogue meter
  updateTime = millis(); // Next update time
}


void loop() {
  buttonB.loop();
  if (updateTime <= millis()) {
    updateTime = millis() + 1; // Update meter every 35 milliseconds
    value[0] = bufCounts;
    plotNeedle(value[0], 0); // It takes between 2 and 14ms to replot the needle with zero delay
    unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > LOG_PERIOD) {
    previousMillis = currentMillis;
    CPMArray[currentCPM] = counts * 2;
    correctCPM = bufCounts * 60;
    Serial.println(String(correctCPM));
    tft.setTextSize(2);
    tft.setCursor(150, 0);
    tft.print("uSv/hr: ");
    tft.setCursor(150, 16);
    tft.print(averageCPM);
    tft.setTextSize(1);
    if (counts > maxCPM){
      maxCPM = counts;
    }
    if ((bufCounts != counts) && (currentMillis = 650) or (currentMillis = 950) or (currentMillis = 1250)){ //This buffers out the current CPM to hold until next LOG_PERIOD to prevent meter jumping
      bufCounts = counts;
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
    

    Serial.println("Avg uSv: " + String(averageCPM) + " +/- " + String(sdCPM) + "  ArrayVal: " + String(CPMArray[currentCPM]));
    currentCPM = currentCPM + 1;
    displayAverageCPM();
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        tft.setTextSize(2);
        tft.setCursor(0, 16);
        String voltage = "V+ :" + String(battery_voltage) + "V";
        Serial.print(voltage);
        tft.setTextDatum(MC_DATUM);
        tft.println(voltage);
        tft.setTextSize(1);
        Serial.print(correctCPM);
    
    }
  }
}
  }


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  tft.setTextColor(TFT_GREEN);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*150;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*150;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE*100 + tl) + M_SIZE*120;
    int y2 = sy2 * (M_SIZE*100 + tl) + M_SIZE*150;
    int x3 = sx2 * M_SIZE*100 + M_SIZE*120;
    int y3 = sy2 * M_SIZE*100 + M_SIZE*150;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
    //  tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    //}

    // Green zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*150;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*150;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_GREEN);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl + 10) + M_SIZE*150;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0+4, y0-4, 1); break;
        case -1: tft.drawCentreString("25", x0+2, y0, 1); break;
        case 0: tft.drawCentreString("50", x0, y0, 1); break;
        case 1: tft.drawCentreString("75", x0, y0, 1); break;
        case 2: tft.drawCentreString("100", x0-2, y0-4, 1); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*150;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_GREEN);
  }
  //tft.drawRect(1, M_SIZE*3, M_SIZE*236, M_SIZE*126, TFT_BLACK); // Draw bezel line
   tft.drawCentreString("CPS", M_SIZE*120, M_SIZE*75, 4); // Comment out to avoid font 4
  //plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  char buf[8]; dtostrf(value, 4, 0, buf);
  char maxcpmbuf[8]; dtostrf(maxCPM, 4, 0, maxcpmbuf); 
  tft.drawRightString(maxcpmbuf, M_SIZE*(3 + 230 - 40), M_SIZE*(119 - 20), 2); // Max CPM this session in Right corner M_SIZE*(119 - 20)M_SIZE*(3 + 230 - 40)
  tft.drawRightString(buf, 33, M_SIZE*(119 - 20), 2); // Current Needle Value

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle until new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calculate tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);
    

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(M_SIZE*(120 + 24 * ltx) - 1, M_SIZE*(150 - 24), osx - 1, osy, TFT_BLACK);
    tft.drawLine(M_SIZE*(120 + 24 * ltx), M_SIZE*(150 - 24), osx, osy, TFT_BLACK);
    tft.drawLine(M_SIZE*(120 + 24 * ltx) + 1, M_SIZE*(150 - 24), osx + 1, osy, TFT_BLACK);

    // Re-plot text under needle
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("CPS", M_SIZE*120, M_SIZE*75, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = M_SIZE*(sx * 98 + 120);
    osy = M_SIZE*(sy * 98 + 150);

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(M_SIZE*(120 + 24 * ltx) - 1, M_SIZE*(150 - 24), osx - 1, osy, TFT_RED);
    tft.drawLine(M_SIZE*(120 + 24 * ltx), M_SIZE*(150 - 24), osx, osy, TFT_MAGENTA);
    tft.drawLine(M_SIZE*(120 + 24 * ltx) + 1, M_SIZE*(150 - 24), osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}
void impulse() {
    counts++;
  }
  void displayAverageCPM()  {
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.print("CPM: ");
    tft.print(correctCPM);
    //tft.print(outputSieverts(averageCPM));
    //tft.print("+/-");
    //tft.println(outputSieverts(sdCPM));
    tft.setTextSize(1);
  }
  float outputSieverts(float x)  {
    float y = x * 0.0057;
    return y;
  }
  void longpress(Button2& btn) {
    unsigned int time = btn.wasPressedFor();
    Serial.print("DeepSleep Init");
    if (time > 1000) {
     esp_deep_sleep_start();     
    }
}
