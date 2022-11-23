#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EEPROM.h>
#include <MQTT.h>

#include "config.h"
#include "schedule.h"

EthernetClient net;
EthernetUDP Udp;
MQTTClient mqtt;

struct schedule sched[16];
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets


unsigned long lastMillis = 0;
unsigned char ScheduleFlag = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial){
    ;
  }
  Serial.println("COLD START");
  ScheduleFlag = ReadScheduleFlag();
#ifdef _DEBUG
  Serial.print("EEPROM = 0x");
  Serial.println(EEPROM.length(), HEX);
  Serial.print("Schedule = ");
  Serial.println(sizeof(sched));
  Serial.print("Schedule Flag = 0x");
  Serial.println(ScheduleFlag, HEX);
#endif
  if(ScheduleFlag == 0xFF)
    ZeroEEPROM();
    
  Serial.println("Ethernet Startup");
  Ethernet.init(ETHERNET_CS);
  if(Ethernet.begin(mac) == 0) {
    Serial.println("Ethernet init failed");
    if(Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Did not find working Ethernet controller");
    }
    else
      if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("No connection");
      }
      while(1)
        delay(1);
  }
  Serial.print("My IP Address: ");
  Serial.print(Ethernet.localIP());
  Serial.print("/");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(Ethernet.dnsServerIP());

  Udp.begin(localPort);

#ifdef _DEBUG
  Serial.println("Checking RTC against NTP");
#endif
  CheckTime();

  Serial.println("MQTT Startup");
  mqtt.begin(MQTT_HOST, net);
  mqtt.onMessage(messageReceived);
  connect();

  Serial.println("I/O Startup");
  pinMode(RLY00, OUTPUT);
  digitalWrite(RLY00, RELAY_OFF);
  pinMode(RLY01, OUTPUT);
  digitalWrite(RLY01, RELAY_OFF);
  pinMode(RLY02, OUTPUT);
  digitalWrite(RLY02, RELAY_OFF);
  pinMode(RLY03, OUTPUT);
  digitalWrite(RLY03, RELAY_OFF);
  pinMode(RLY04, OUTPUT);
  digitalWrite(RLY04, RELAY_OFF);
  pinMode(RLY05, OUTPUT);
  digitalWrite(RLY05, RELAY_OFF);
  pinMode(RLY06, OUTPUT);
  digitalWrite(RLY06, RELAY_OFF);
  pinMode(RLY07, OUTPUT);
  digitalWrite(RLY07, RELAY_OFF);
  pinMode(RLY08, OUTPUT);
  digitalWrite(RLY08, RELAY_OFF);
  pinMode(RLY09, OUTPUT);
  digitalWrite(RLY09, RELAY_OFF);
  pinMode(RLY10, OUTPUT);
  digitalWrite(RLY10, RELAY_OFF);
  pinMode(RLY11, OUTPUT);
  digitalWrite(RLY11, RELAY_OFF);
  pinMode(RLY12, OUTPUT);
  digitalWrite(RLY12, RELAY_OFF);
  pinMode(RLY13, OUTPUT);
  digitalWrite(RLY13, RELAY_OFF);
  pinMode(RLY14, OUTPUT);
  digitalWrite(RLY14, RELAY_OFF);
  pinMode(RLY15, OUTPUT);
  digitalWrite(RLY15, RELAY_OFF);
}

void loop() {
  switch(Ethernet.maintain()) {
    case 1: // Renewal failed
      Serial.println("DHCP Renewal failed");
      break;
    case 2: // Renewal success
      Serial.println("DHCP Renewal success");
      break;
    case 3: // Rebind failed
      Serial.println("Rebind failed");
      break;
    case 4: // Rebind success
      Serial.println("Rebind success");
      break;
  }
#ifdef _DEBUG  
  Serial.println("Run mqtt.loop()");
#endif  
  mqtt.loop();

  if(!mqtt.connected()) {
    connect();
  }

  CheckSchedule();

  if(millis() - lastMillis > 60000) { // Approx 1 minute
    lastMillis = millis();
    mqtt.publish("/status", "hello");
#ifdef _DEBUG
    Serial.println("Publish");
#endif    
  }
  delay(60000);
}

void connect() {
  Serial.print("Connecting to ");
  Serial.println(MQTT_HOST);

  while(!mqtt.connect(MY_ID, MY_ID, MY_PASS)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

unsigned char ReadScheduleFlag() {
  return EEPROM.read(EEPROM.length() -1);
}

void SetScheduleFlag() {
  EEPROM.write(EEPROM.length() - 1, 1);
}

void ClearScheduleFlag() {
  EEPROM.write(EEPROM.length() - 1, 0);
}

void ZeroEEPROM() {
  int i = 0;
  for(i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void CheckSchedule() {
  unsigned char ControlSegment = 0xFF;
#ifdef _DEBUG
  Serial.println("In CheckSchedule");
#endif
  unsigned long epoch = CheckTime();
  if(epoch != 0) {
    unsigned hours = (epoch  % 86400L) / 3600;
    unsigned long minutes = (epoch % 3600) / 60;
#ifdef _DEBUG
      Serial.print("CheckSchedule: minutes = ");
      Serial.println(minutes);
      
#endif
    if((minutes == 30) || (minutes == 0)) {
#ifdef _DEBUG
      Serial.println("Time to run a schedule");
#endif
      switch(hours) {
        case 0:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0000;
          }
          else {
            ControlSegment = sched[0].slot0030;
          }
        case 1:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0100;
          }
          else {
            ControlSegment = sched[0].slot0130;
          }
        case 2:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0200;
          }
          else {
            ControlSegment = sched[0].slot0230;
          }
        case 3:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0300;
          }
          else {
            ControlSegment = sched[0].slot0330;
          }
        case 4:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0400;
          }
          else {
            ControlSegment = sched[0].slot0430;
          }
        case 5:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0500;
          }
          else {
            ControlSegment = sched[0].slot0530;
          }
        case 6:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0600;
          }
          else {
            ControlSegment = sched[0].slot0630;
          }
        case 7:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0700;
          }
          else {
            ControlSegment = sched[0].slot0730;
          }
        case 8:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0800;
          }
          else {
            ControlSegment = sched[0].slot0830;
          }
        case 9:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0900;
          }
          else {
            ControlSegment = sched[0].slot0930;
          }
        case 10:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1000;
          }
          else {
            ControlSegment = sched[0].slot1030;
          }
        case 11:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1100;
          }
          else {
            ControlSegment = sched[0].slot1130;
          }
        case 12:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1200;
          }
          else {
            ControlSegment = sched[0].slot1230;
          }
        case 13:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1300;
          }
          else {
            ControlSegment = sched[0].slot1330;
          }
        case 14:
          if(minutes == 0) {
            ControlSegment = sched[0].slot0400;
          }
          else {
            ControlSegment = sched[0].slot0430;
          }
        case 15:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1500;
          }
          else {
            ControlSegment = sched[0].slot1530;
          }
        case 16:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1600;
          }
          else {
            ControlSegment = sched[0].slot1630;
          }
        case 17:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1700;
          }
          else {
            ControlSegment = sched[0].slot1730;
          }
        case 18:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1800;
          }
          else {
            ControlSegment = sched[0].slot1830;
          }
        case 19:
          if(minutes == 0) {
            ControlSegment = sched[0].slot1900;
          }
          else {
            ControlSegment = sched[0].slot1930;
          }
        case 20:
          if(minutes == 0) {
            ControlSegment = sched[0].slot2000;
          }
          else {
            ControlSegment = sched[0].slot2030;
          }
        case 21:
          if(minutes == 0) {
            ControlSegment = sched[0].slot2100;
          }
          else {
            ControlSegment = sched[0].slot2130;
          }
        case 22:
          if(minutes == 0) {
            ControlSegment = sched[0].slot2200;
          }
          else {
            ControlSegment = sched[0].slot2230;
          }
        case 23:
          if(minutes == 0) {
            ControlSegment = sched[0].slot2300;
          }
          else {
            ControlSegment = sched[0].slot2330;
          }
      }
#ifdef _DEBUG
      Serial.print("Control Segment = 0x");
      Serial.println(ControlSegment, HEX);
#endif  
    }
#ifdef _DEBUG
  else
    Serial.println("CheckSchedule: Nothing to do");
#endif  
  }
}

unsigned long CheckTime() {
  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch = 0L;
  
  sendNTPpacket(NTP_HOST); // send an NTP packet to a time server
  delay(1000); // wait for a reply
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    epoch = secsSince1900 - seventyYears;
#ifdef _DEBUG
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);
    Serial.print("Unix time = ");
    Serial.println(epoch);
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) {
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
#endif    
  }
  return epoch;
}

void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
