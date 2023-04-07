#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_SleepyDog.h>
#include <Arduino_JSON.h>
#include <CRC32.h>
#include "TimeLib.h"

String readBuffer = "";

// Timekeeping
unsigned long sendData = 0;         // next time we'll send data
unsigned long SEND_WAIT = 3600;     // how long to wait between submissions -- 1800 = 30 min
unsigned long LOOP_WAIT_MS = 100;  // how long to wait between loops -- 2000 ms = 2 sec
unsigned long lastLoopMillis = 0; // time of last loop execution

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  int countdownMS = Watchdog.enable(16000); // 16s is max timeout
  
  Serial.begin(9600);
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
    
  delay(5000);

  Serial.println("LoRa Receiver");

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.enableCrc();
  Serial.println("LoRa started - waiting for data...");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - lastLoopMillis >= LOOP_WAIT_MS)
  {  
    Watchdog.reset();
    digitalWrite(LED_BUILTIN, HIGH);

    Watchdog.reset();
    // try to parse packet
    int packetSize = LoRa.parsePacket();
    Watchdog.reset();
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");
  
      // read packet
      while (LoRa.available()) {
        int msg = LoRa.read();
        readBuffer += (char)msg;
      }
      Watchdog.reset();
      Serial.print(readBuffer);
  
      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
      Watchdog.reset();

      JSONVar sample = validateSampleAndCreateJSON(readBuffer);
      Serial.println("Got sample with length " + String(JSON.stringify(sample).length()));
      if(JSON.stringify(sample).length() > 0)
      {
        Watchdog.reset();
        Serial.println("Sending ACK back");
        // send packet
        LoRa.beginPacket();
        LoRa.print("ACK");
        LoRa.endPacket();

        String postData = JSON.stringify(sample);
        //TODO: SEND TO INTERNET
        Serial.println("BEGIN DATA");
        Serial.println(postData);
        Serial.println("END");
        
      } else {
        Serial.println("Sample '" + readBuffer + "' discarded");
      }
      readBuffer = "";
      Watchdog.reset();
    }
    Watchdog.reset();
    digitalWrite(LED_BUILTIN, LOW);
    lastLoopMillis = millis(); // set the timing for the next loop
    Watchdog.reset();
  }
}

String formatDateTime(time_t t)
{
    int y = year(t);
    int mn = month(t);
    int d = day(t);
    int h = hour(t);
    int mi = minute(t);
    int s = second(t);

    String y_s = String(y);
    String mn_s = mn > 9 ? String(mn) : ("0" + String(mn));
    String d_s = d > 9 ? String(d) : ("0" + String(d));
    String h_s = h > 9 ? String(h) : ("0" + String(h));
    String mi_s = mi > 9 ? String(mi) : ("0" + String(mi));
    String s_s = s > 9 ? String(s) : ("0" + String(s));

    String retval = y_s + "-" + mn_s + "-" + d_s + " " + h_s + ":" + mi_s + ":" + s_s + " UTC";

    return retval;
}

JSONVar validateSampleAndCreateJSON(String sampleString) 
{
  JSONVar sample;
  char buf[255];
  sampleString.toCharArray(buf, 255);
  char *p = buf;
  char *str;
  int i = 0;
  bool validSample = false;
  bool validTemp = false;
  String tempString = "";
  String voltageString = "";
  
  Watchdog.reset();
  Serial.println("Tokenizing '" + String(p) + "'");
  while ((str = strtok_r(p, ";", &p)) != NULL && i < 3) // delimiter is the semicolon
  {
    char *end;
    double val = strtod(str, &end);
    if(str != end) {
      if(i == 0) 
      {
        sample["temperature"] = String(val);
        validTemp = true;
        tempString = String(val,1);
      } 
      else if (i == 1) 
      {
        sample["voltage"] = String(val);
        if(validTemp) { // we have both temp and voltage -- likely a good read
          validSample = true;
          voltageString = String(val,1);
        }
      } 
      else if (i == 2)
      {
        Serial.println("CRC: " + String(str));
        unsigned int crc_received = atoi(str);        
        String toCheck = tempString + ";" + voltageString;
        Serial.println("Get CRC of '" + toCheck + "'");
        unsigned int checksum = computeCRC(toCheck);
        if(crc_received != checksum)
        {
          Serial.println("Checksum " + String(checksum) + " doesn't match checksum in transmission (" + String(crc_received) + ")");
          validSample = false;
        } else {
          Serial.println("Checksum " + String(checksum) + " is ok!");
          validSample = true;
        }
      } 
      else 
      {
        Serial.println("Extra data received [" + String(i) + "]: " + String(str));
      }  
    } else {
      Serial.println("Invalid value received [" + String(i) + "]: " + String(str));
    }
    i++;
  }
  if(validSample) 
  {
    Serial.println("Valid sample, returning sample");
    return sample;
  } else {
    Serial.println("Invalid sample, returning new JSONVar");
    return new JSONVar;
  }
}

uint32_t computeCRC(String input)
{
  CRC32 crc;
  char stringToCRC[255];
  int len = input.length() + 1;
  if(len > 255)
    len = 254;
  
  input.toCharArray(stringToCRC, len);
  
  crc.add((uint8_t*)stringToCRC, len);
  return crc.getCRC();
}
