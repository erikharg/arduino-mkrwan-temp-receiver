#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_SleepyDog.h>
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
    }
    Watchdog.reset();
    digitalWrite(LED_BUILTIN, LOW);
    lastLoopMillis = millis(); // set the timing for the next loop
    Watchdog.reset();
    readBuffer = "";
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
