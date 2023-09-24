/*-------------------------------------------------

              Client Transmitter Code

---------------------------------------------------*/


/*--------------------------------------------------
    *Feather M0 RF9X
    *RF95 (LoRa) 868 or 915 Mhz
    *ADXL335 Accelerometer
    *DHT Humidity and Temperature Sensor
    *16-bit RGB LED Ring Light
----------------------------------------------------*/


#include <SPI.h>
#include <RH_RF95.h>

#include "DHT.h"

#include <math.h>


#include <FastLED.h>



#define RF95_FREQ 868.0

//RF95 Setup
RH_RF95 driver(8, 3); // Adafruit Feather M0 with RFM95 


//ADAXL335 Accelerometer Pins
#define X_OUT       A2 /* connect X_OUT of module to A1 of UNO board */
#define Y_OUT       A1 /* connect Y_OUT of module to A2 of UNO board */
#define Z_OUT       A0 /* connect Z_OUT of module to A3 of UNO board */



//RGB LED Ring Light
#define LED_PIN     11
#define NUM_LEDS    16
#define Brightness  255 // 0 - 255
CRGB leds[NUM_LEDS];



//DHT Temperature and Humidity Sensor
#define DHTPIN      12
#define DHTTYPE     DHT11
DHT dht(DHTPIN, DHTTYPE);


//Emergency Button
#define BUTTON_PIN 6


/* Incrementing varaiable used to keep track of packet index -- 
used to determine what data to send */
int count = 0;


//Variable keeping track of whether the helmet/user has fallen
int fallenState = 0;


void setup() 
{
  //Pin setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(X_OUT, INPUT);
  pinMode(Y_OUT, INPUT);
  pinMode(Z_OUT, INPUT);

  pinMode(LED_PIN, OUTPUT);

  //DHT Sensor Init
  dht.begin();



  
  Serial.begin(9600);


  //Initializing RF95 Module
  if (!driver.init())
    Serial.println("init failed");

  //Setting RF95 Frequency
  if (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  driver.setTxPower(15, false);

  //LED Ring Light Setup
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{


  //ADXL335 Accelerometer -- Reading values
  int x_adc_value, y_adc_value, z_adc_value; 
  double x_g_value, y_g_value, z_g_value;
  double roll, pitch, yaw;
  x_adc_value = analogRead(X_OUT); //Digital value of voltage on X_OUT pin 
  y_adc_value = analogRead(Y_OUT); // Digital value of voltage on Y_OUT pin 
  z_adc_value = analogRead(Z_OUT); ///Digital value of voltage on Z_OUT pin 
  x_g_value = ( ( ( (double)(x_adc_value * 5)/1024) - 1.65 ) / 0.330 ); // Acceleration in x-direction in g units 
  y_g_value = ( ( ( (double)(y_adc_value * 5)/1024) - 1.65 ) / 0.330 ); //Acceleration in y-direction in g units 
  z_g_value = ( ( ( (double)(z_adc_value * 5)/1024) - 1.80 ) / 0.330 ); // Acceleration in z-direction in g units 
  roll = ( ( (atan2(y_g_value,z_g_value) * 180) / 3.14 ) + 180 ); // Formula for roll 
  pitch = ( ( (atan2(z_g_value,x_g_value) * 180) / 3.14 ) + 180 ); // Formula for pitch 
  

  //Determining fallenState based on roll and pitch
  if (roll < 235 || pitch > 230 || z_g_value > 1.5){
    fallenState = 1;

  } else{
    fallenState = 0;
  }



  //Reading DHT Sensor values 
  float humidity = dht.readHumidity();       
  float temp_c = dht.readTemperature();          
  float temp_f = dht.readTemperature(true);
  float hif = dht.computeHeatIndex(temp_f, humidity);
  float hic = dht.computeHeatIndex(temp_c, humidity, false);


  //Setting LED RInglight values based on Emergency Button, Tilt, and Acceleration
  if (digitalRead(BUTTON_PIN) == 1){
    fill_solid(leds, NUM_LEDS, CRGB::Red);
  }
  else if (fallenState == 1){
    fill_solid(leds, NUM_LEDS, CRGB::Purple);
  }
  else{
      fill_solid(leds, NUM_LEDS, CRGB::White);
  }
  FastLED.setBrightness(Brightness); // maximum brightness between 0 and 255
  FastLED.show();



  /*------------------------------------
          Sending Packets Thru RF
  ------------------------------------*/
  Serial.println("Sending to rf95_server" + String(count));

  if (count == 0){
    Serial.println("Sending id");
    uint8_t data[] = "ID01";
    driver.send(data, sizeof(data));
  }

  if (count == 1){
    Serial.println("Sending button state: " + String(digitalRead(BUTTON_PIN)));
    sendInt(digitalRead(BUTTON_PIN));
  }
  

  if (count == 2) {
    Serial.println("Sending pitch: " + String(pitch));
    sendInt(pitch);
  }

  if (count == 3){
    Serial.println("sending celsius temperature:" + String(temp_c));
    sendInt(temp_c);
  }

  if (count == 4){
    Serial.println("sending fahrenheit temperature:" + String(temp_f));
    sendInt(temp_f);
  }

  if (count == 5){
    Serial.println("sending humidity" + String(humidity));
    sendInt(humidity);
  }

  if (count == 6){
    Serial.println("Sending fallen state");
    sendInt(fallenState);
    
  }

  if (count == 7){
    Serial.println("Sending roll: " + String(roll));
    sendInt(roll);
    count = -1;
  }


  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (driver.waitAvailableTimeout(3000))
  { 
    //Waiting for a reply/acknowledgement from the server 
    if (driver.recv(buf, &len))
    {
     //Only increments count if the server acknowledges that it received a value
      count += 1;
      Serial.print("got reply: ");
      Serial.println((char*)buf);
    } 

    else
    {
      Serial.println("recv failed");
    }

  }

  else
  {
    //Client trying to transmit data, server not connecting
    Serial.println("No reply, is rf95_server running?");
  }


  delay(20);
}


void sendInt(int num){
  char a[20];
  itoa(num, a, 10);
  driver.send((uint8_t*)a, sizeof(a));
  Serial.println(a);
  driver.waitPacketSent();
  
}