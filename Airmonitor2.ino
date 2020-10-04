#include <Wire.h>

#include <Adafruit_Sensor.h> // for BME280
#include <Adafruit_BME280.h> // for BME280
#include "SparkFunCCS811.h" 

#define LCD_RST 13   //D7 :GPIO13
#define SCLPIN   5   //D1 :GPIO5
#define SDAPIN   4   //D2 :GPIO4
#define CSS_RST 14   //D5 :GPIO14

#define SEALEVELPRESSURE_HPA (1013.25)
//#define CCS811_ADDR 0x5B //ADD='H'
#define CCS811_ADDR 0x5A // ADD='L'

Adafruit_BME280 bme; // I2C
CCS811 myCCS811(CCS811_ADDR);

String lcdbuf1,lcdbuf2;

void AQMwriteCommand(byte command)
{
  Wire.beginTransmission(0x3E);
  Wire.write(0x00);
  Wire.write(command);
  Wire.endTransmission();delay(10);
}

void AQMwriteData(byte data)
{
  Wire.beginTransmission(0x3E);
  Wire.write(0x40);
  Wire.write(data);
  Wire.endTransmission();delay(1);
}

void AQMbufTransfer(String lcdbuf1, String lcdbuf2){
    AQMwriteCommand(0x80);//DDRAMアドレスを2行目先頭にセット
    for(int i = 0; i < 8; i++){
      AQMwriteData( lcdbuf1[i] );
    }
    AQMwriteCommand(0x40+0x80);//DDRAMアドレスを2行目先頭にセット
    for(int i = 0; i < 8; i++){
      AQMwriteData(lcdbuf2[i]);
    }
}

void SerialDump(float tempC,float humid,float pressure, float altiture, float CO2, float TVOC) {
    Serial.print("Temperature = ");
    Serial.print(tempC);
    Serial.println(" *C");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("Approx. Altitude = ");
    Serial.print(altiture);
    Serial.println(" m");
    Serial.print("Humidity = ");
    Serial.print(humid);
    Serial.println(" %");
    Serial.print("CO2[");  
    Serial.print(CO2);
    Serial.print("] tVOC[");
    Serial.print(TVOC);
    Serial.print("] millis[");
    Serial.print(millis());
    Serial.print("]");
    Serial.println();
}


void setup() {
  unsigned status;
  
  pinMode(LCD_RST, OUTPUT);
  pinMode(CSS_RST, OUTPUT);
  digitalWrite(LCD_RST, LOW);
  digitalWrite(CSS_RST, LOW);
  delay(1500);
  digitalWrite(LCD_RST, HIGH);
  digitalWrite(CSS_RST, HIGH);
  delay(1500);
  
  Wire.begin(SDAPIN,SCLPIN);
  delay(100);
  AQMwriteCommand(0x38);delay(20);
  AQMwriteCommand(0x39);delay(20); 
  AQMwriteCommand(0x14);delay(20);
  AQMwriteCommand(0x73);delay(20);//3.3V=0x73, 5V=0x7A
  AQMwriteCommand(0x56);delay(20);//3.3V=0x56, 5V=0x54
  AQMwriteCommand(0x6C);delay(20);
  AQMwriteCommand(0x38);delay(20);
  AQMwriteCommand(0x01);delay(20);
  AQMwriteCommand(0x0C);delay(20);

  Serial.begin(115200);
  while(!Serial);    // time to get serial running
  Serial.print("start.");

  delay(100);
  status = bme.begin();
      if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1);
    }

  if (myCCS811.begin() == false){
    Serial.print("CCS811 error. Please check wiring. Freezing...");
    while (1);
  }
}

byte  dispmode=0;

void loop() {
    float tempC,humid,pressure,altitude, CO2,TVOC;

    tempC = bme.readTemperature();
    humid = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;
    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    if (myCCS811.dataAvailable())
    {
      //If so, have the sensor read and calculate the results.
      //Get them later
      myCCS811.readAlgorithmResults();
      myCCS811.setEnvironmentalData(humid, tempC);
      delay(2000); //Wait for next reading
      CO2  = myCCS811.getCO2();
      TVOC = myCCS811.getTVOC();
  
    }

    SerialDump(tempC, humid, pressure, altitude, CO2, TVOC);

    if (dispmode==0) {
      char degree = 0xdf;  // "°"
      lcdbuf1 = String(tempC,1)  + degree + "C";
      lcdbuf2 = String(humid,0) + "%";
      dispmode=1;
    } else if (dispmode==1) {
      lcdbuf1 = String(pressure,0) + "hPa";
      lcdbuf2 = String(altitude,0) + "m";
      dispmode=2;
    } else {
      lcdbuf1 = "CO2 " + String(CO2,0);
      lcdbuf2 = "TVOC " + String(TVOC,0);
      dispmode=0;
    }    
    AQMbufTransfer(lcdbuf1, lcdbuf2);

    delay(1000);
}
  
