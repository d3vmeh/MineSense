/*-------------------------------------------------

            Server Transmitter Code

---------------------------------------------------*/


#include <SPI.h>
#include <RH_RF95.h>
#include <string.h>
#include "DHT.h"
#include <math.h>



//RF95 Setup
#define RF95_FREQ 868.0
RH_RF95 driver(8, 3); // Adafruit Feather M0 with RFM95 





/* Incrementing varaiable used to keep track of packet index -- 
used to determine what data is expected to be received */
int count = -1;

//Total amount of data sent in each packet
int maxcount = 7; 



bool receivedID = false;

typedef struct Transmitter{
  char id[3]; //4 character code, starts with prefix "ID", followed by 2 digits for identification
  int emergencyButtonState = 0; //when 1, button is pressed. when 0, button unpressed
  int pitch;
  float tempC;
  float tempF;
  float humidity;
  int fallenState;
  int roll;
  
} Transmitter;


Transmitter helmet;




void setup() 
{
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  pinMode(LED_BUILTIN, OUTPUT);     
  Serial.begin(9600);



  //Radio Init

  //while (!Serial) ; // Wait for serial port to be available REMOVE OR UNCOMMENT WHEN NEEDED -- REQUIRES SERIAL MONITOR TO BE OPEN
  if (!driver.init())
    Serial.println("init failed");  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  if (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  driver.setTxPower(15, false);


  //For communicating with MSP432 via UART
  Serial1.begin(115200);
}

void loop()
{

  //Serial.println(potvalue);
  if (driver.available())
  {
    //Serial.println("avaialble");
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (driver.recv(buf, &len))
    {
      digitalWrite(LED_BUILTIN, HIGH);
      char s[len];
      strcpy(s, (char*)buf);  
      
      if (receivedID){
        count += 1; 
      }

      //Check values

      //Waiting to receive an ID value to identify the helmet, only then will the server receive any information
      if (count == 0 || count == -1){
        strcpy(s, (char*)buf);
        //id stored in s
        strcpy(helmet.id, s);
        if (count == -1){
          count = 0;
        }

        //To be adajusted to support multiple helmets 
        if (helmet.id[0] == 'I'){
          receivedID = true;
        }

      }

      if (count == 1 && receivedID){  
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.emergencyButtonState = n;
      }


      //Pitch
      if (count == 2 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.pitch = n;
      }

      //Temperature in Celsius
      if (count == 3 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.tempC = n;
      }    

      //Temperature in Fahrenheit
      if (count == 4 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.tempF = n;
      }    

      //Humidity
      if (count == 5 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.humidity = n;
      }    

      if (count == 6 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.fallenState = n;
      }  

      if (count == 7 && receivedID){
        char s[len];
        strcpy(s, (char*)buf);
        int n;
        n = atoi(s);
        helmet.roll = n;
      }  


    //Displaying Values
      if (count >= maxcount && receivedID){
        count = -1;
        Serial.print("ID: ");
        Serial.println(helmet.id);
        Serial.print("Emergency Button State: ");
        Serial.println(helmet.emergencyButtonState);
        Serial.print("Temperature C: ");
        Serial.println(helmet.tempC);
        Serial.print("Temperature F: ");
        Serial.println(helmet.tempF);
        Serial.print("Humidity: ");
        Serial.print(helmet.humidity);
        Serial.println("%");
        Serial.print("Pitch: ");
        Serial.println(helmet.pitch);
        Serial.print("Roll: ");
        Serial.println(helmet.roll);
        Serial.print("Fallen?: ");
        Serial.println(helmet.fallenState);
        Serial.println("");
        Serial.println("");
        

        receivedID = false;
        strcpy(helmet.id, "");
      }

      if (count > maxcount){
        count = -1;
      }

      // Send a reply acknowledging the server has received data
      uint8_t data[] = "Received data";
      driver.send(data, sizeof(data));
      driver.waitPacketSent();
      
      
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      Serial.println("recv failed");
    }
  }

}