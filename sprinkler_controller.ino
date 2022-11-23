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
  Serial.print("Sched Block = ");
  Serial.println(sizeof(struct schedule));
  Serial.print("Schedule = ");
  Serial.println(sizeof(sched));
  Serial.print("Schedule Flag = 0x");
  Serial.println(ScheduleFlag, HEX);
#endif
  switch(ScheduleFlag) {
    case 0x01:
      ReadSchedFromEEPROM();
      break;
    case 0xFF:
      ZeroEEPROM();
      break;
  }
    
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
  delay(59000);   // allow for the 1 second in ntp read
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
  int i = 0;
  
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
    if((minutes == 45) || (minutes == 30) || (minutes == 15) || (minutes == 0)) {
      if(hours = 0) {
        WriteSchedToEEPROM();
      }
#ifdef _DEBUG
      Serial.println("Time to run a schedule");
#endif
    for(i = 0; i < 16; i++) {
        switch(hours) {
          case 0:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0000;
                break;
              case 15:
                ControlSegment = sched[i].slot0015;
                break;
              case 30:
                ControlSegment = sched[i].slot0030;
                break;
              case 45:
                ControlSegment = sched[i].slot0045;
                break;
            }
          case 1:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0100;
                break;
              case 15:
                ControlSegment = sched[i].slot0115;
                break;
              case 30:
                ControlSegment = sched[i].slot0130;
                break;
              case 45:
                ControlSegment = sched[i].slot0145;
                break;
            }
          case 2:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0200;
                break;
              case 15:
                ControlSegment = sched[i].slot0215;
                break;
              case 30:
                ControlSegment = sched[i].slot0230;
                break;
              case 45:
                ControlSegment = sched[i].slot0245;
                break;
            }
          case 3:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0300;
                break;
              case 15:
                ControlSegment = sched[i].slot0315;
                break;
              case 30:
                ControlSegment = sched[i].slot0330;
                break;
              case 45:
                ControlSegment = sched[i].slot0345;
                break;
            }
          case 4:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0400;
                break;
              case 15:
                ControlSegment = sched[i].slot0415;
                break;
              case 30:
                ControlSegment = sched[i].slot0430;
                break;
              case 45:
                ControlSegment = sched[i].slot0445;
                break;
            }
          case 5:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0500;
                break;
              case 15:
                ControlSegment = sched[i].slot0515;
                break;
              case 30:
                ControlSegment = sched[i].slot0530;
                break;
              case 45:
                ControlSegment = sched[i].slot0545;
                break;
            }
          case 6:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0600;
                break;
              case 15:
                ControlSegment = sched[i].slot0615;
                break;
              case 30:
                ControlSegment = sched[i].slot0630;
                break;
              case 45:
                ControlSegment = sched[i].slot0645;
                break;
            }
          case 7:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0700;
                break;
              case 15:
                ControlSegment = sched[i].slot0715;
                break;
              case 30:
                ControlSegment = sched[i].slot0730;
                break;
              case 45:
                ControlSegment = sched[i].slot0745;
                break;
            }
          case 8:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0800;
                break;
              case 15:
                ControlSegment = sched[i].slot0815;
                break;
              case 30:
                ControlSegment = sched[i].slot0830;
                break;
              case 45:
                ControlSegment = sched[i].slot0845;
                break;
            }
          case 9:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot0900;
                break;
              case 15:
                ControlSegment = sched[i].slot0915;
                break;
              case 30:
                ControlSegment = sched[i].slot0930;
                break;
              case 45:
                ControlSegment = sched[i].slot0945;
                break;
            }
          case 10:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1000;
                break;
              case 15:
                ControlSegment = sched[i].slot1015;
                break;
              case 30:
                ControlSegment = sched[i].slot1030;
                break;
              case 45:
                ControlSegment = sched[i].slot1045;
                break;
            }
          case 11:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1100;
                break;
              case 15:
                ControlSegment = sched[i].slot1115;
                break;
              case 30:
                ControlSegment = sched[i].slot1130;
                break;
              case 45:
                ControlSegment = sched[i].slot1145;
                break;
            }
          case 12:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1200;
                break;
              case 15:
                ControlSegment = sched[i].slot1215;
                break;
              case 30:
                ControlSegment = sched[i].slot1230;
                break;
              case 45:
                ControlSegment = sched[i].slot1245;
                break;
            }
          case 13:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1300;
                break;
              case 15:
                ControlSegment = sched[i].slot1315;
                break;
              case 30:
                ControlSegment = sched[i].slot1330;
                break;
              case 45:
                ControlSegment = sched[i].slot1345;
                break;
            }
          case 14:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1400;
                break;
              case 15:
                ControlSegment = sched[i].slot1415;
                break;
              case 30:
                ControlSegment = sched[i].slot1430;
                break;
              case 45:
                ControlSegment = sched[i].slot1445;
                break;
            }
          case 15:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1500;
                break;
              case 15:
                ControlSegment = sched[i].slot1515;
                break;
              case 30:
                ControlSegment = sched[i].slot1530;
                break;
              case 45:
                ControlSegment = sched[i].slot1545;
                break;
            }
          case 16:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1600;
                break;
              case 15:
                ControlSegment = sched[i].slot1615;
                break;
              case 30:
                ControlSegment = sched[i].slot1630;
                break;
              case 45:
                ControlSegment = sched[i].slot1645;
                break;
            }
          case 17:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1700;
                break;
              case 15:
                ControlSegment = sched[i].slot1715;
                break;
              case 30:
                ControlSegment = sched[i].slot1730;
                break;
              case 45:
                ControlSegment = sched[i].slot1745;
                break;
            }
          case 18:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1800;
                break;
              case 15:
                ControlSegment = sched[i].slot1815;
                break;
              case 30:
                ControlSegment = sched[i].slot1830;
                break;
              case 45:
                ControlSegment = sched[i].slot1845;
                break;
            }
          case 19:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot1900;
                break;
              case 15:
                ControlSegment = sched[i].slot1915;
                break;
              case 30:
                ControlSegment = sched[i].slot1930;
                break;
              case 45:
                ControlSegment = sched[i].slot1945;
                break;
            }
          case 20:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot2000;
                break;
              case 15:
                ControlSegment = sched[i].slot2015;
                break;
              case 30:
                ControlSegment = sched[i].slot2030;
                break;
              case 45:
                ControlSegment = sched[i].slot2045;
                break;
            }
          case 21:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot2100;
                break;
              case 15:
                ControlSegment = sched[i].slot2115;
                break;
              case 30:
                ControlSegment = sched[i].slot2130;
                break;
              case 45:
                ControlSegment = sched[i].slot2145;
                break;
            }
          case 22:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot2200;
                break;
              case 15:
                ControlSegment = sched[i].slot2215;
                break;
              case 30:
                ControlSegment = sched[i].slot2230;
                break;
              case 45:
                ControlSegment = sched[i].slot2245;
                break;
            }
          case 23:
            switch(minutes) {
              case 0:
                ControlSegment = sched[i].slot2300;
                break;
              case 15:
                ControlSegment = sched[i].slot2315;
                break;
              case 30:
                ControlSegment = sched[i].slot2330;
                break;
              case 45:
                ControlSegment = sched[i].slot2345;
                break;
            }
          }
#ifdef _DEBUG
      Serial.print("Control Segment[");
      Serial.print(i);
      Serial.print("]= 0x");
      Serial.println(ControlSegment, HEX);
#endif  
      }
    }
#ifdef _DEBUG
  else
    Serial.println("CheckSchedule: Nothing to do");
#endif  
  }
}

void WriteSchedToEEPROM() {
  unsigned int i = 0;
  unsigned int loc = 0;

  for(i = 0; i < 16; i++) {
    loc = 0 + (i * sizeof(struct schedule));
#ifdef _DEBUG
    Serial.print("WriteSchedToEEPROM(): Block write starts at ");
    Serial.println(loc);
#endif
    EEPROM.put(loc, sched[i]);
  }
}

/*
void WriteSchedToEEPROM() {
  unsigned int i = 0;
  unsigned int loc = 0;

  for(i = 0; i < 16; i++) {
    loc = 0 + (i * sizeof(struct schedule));
#ifdef _DEBUG
    Serial.print("WriteSchedToEEPROM(): Block write starts at ");
    Serial.println(loc);
#endif
    EEPROM.update(loc, sched[i].slot0000);
    loc++;
    EEPROM.update(loc, sched[i].slot0015);
    loc++;
    EEPROM.update(loc, sched[i].slot0030);
    loc++;
    EEPROM.update(loc, sched[i].slot0045);
    loc++;
    EEPROM.update(loc, sched[i].slot0100);
    loc++;
    EEPROM.update(loc, sched[i].slot0115);
    loc++;
    EEPROM.update(loc, sched[i].slot0130);
    loc++;
    EEPROM.update(loc, sched[i].slot0145);
    loc++;
    EEPROM.update(loc, sched[i].slot0200);
    loc++;
    EEPROM.update(loc, sched[i].slot0215);
    loc++;
    EEPROM.update(loc, sched[i].slot0230);
    loc++;
    EEPROM.update(loc, sched[i].slot0245);
    loc++;
    EEPROM.update(loc, sched[i].slot0300);
    loc++;
    EEPROM.update(loc, sched[i].slot0315);
    loc++;
    EEPROM.update(loc, sched[i].slot0330);
    loc++;
    EEPROM.update(loc, sched[i].slot0345);
    loc++;
    EEPROM.update(loc, sched[i].slot0400);
    loc++;
    EEPROM.update(loc, sched[i].slot0415);
    loc++;
    EEPROM.update(loc, sched[i].slot0430);
    loc++;
    EEPROM.update(loc, sched[i].slot0445);
    loc++;
    EEPROM.update(loc, sched[i].slot0500);
    loc++;
    EEPROM.update(loc, sched[i].slot0515);
    loc++;
    EEPROM.update(loc, sched[i].slot0530);
    loc++;
    EEPROM.update(loc, sched[i].slot0545);
    loc++;
    EEPROM.update(loc, sched[i].slot0600);
    loc++;
    EEPROM.update(loc, sched[i].slot0615);
    loc++;
    EEPROM.update(loc, sched[i].slot0630);
    loc++;
    EEPROM.update(loc, sched[i].slot0645);
    loc++;
    EEPROM.update(loc, sched[i].slot0700);
    loc++;
    EEPROM.update(loc, sched[i].slot0715);
    loc++;
    EEPROM.update(loc, sched[i].slot0730);
    loc++;
    EEPROM.update(loc, sched[i].slot0745);
    loc++;
    EEPROM.update(loc, sched[i].slot0800);
    loc++;
    EEPROM.update(loc, sched[i].slot0815);
    loc++;
    EEPROM.update(loc, sched[i].slot0830);
    loc++;
    EEPROM.update(loc, sched[i].slot0845);
    loc++;
    EEPROM.update(loc, sched[i].slot0900);
    loc++;
    EEPROM.update(loc, sched[i].slot0915);
    loc++;
    EEPROM.update(loc, sched[i].slot0930);
    loc++;
    EEPROM.update(loc, sched[i].slot0945);
    loc++;
    EEPROM.update(loc, sched[i].slot1000);
    loc++;
    EEPROM.update(loc, sched[i].slot1015);
    loc++;
    EEPROM.update(loc, sched[i].slot1030);
    loc++;
    EEPROM.update(loc, sched[i].slot1045);
    loc++;
    EEPROM.update(loc, sched[i].slot1100);
    loc++;
    EEPROM.update(loc, sched[i].slot1115);
    loc++;
    EEPROM.update(loc, sched[i].slot1130);
    loc++;
    EEPROM.update(loc, sched[i].slot1145);
    loc++;
    EEPROM.update(loc, sched[i].slot1200);
    loc++;
    EEPROM.update(loc, sched[i].slot1215);
    loc++;
    EEPROM.update(loc, sched[i].slot1230);
    loc++;
    EEPROM.update(loc, sched[i].slot1245);
    loc++;
    EEPROM.update(loc, sched[i].slot1300);
    loc++;
    EEPROM.update(loc, sched[i].slot1315);
    loc++;
    EEPROM.update(loc, sched[i].slot1330);
    loc++;
    EEPROM.update(loc, sched[i].slot1345);
    loc++;
    EEPROM.update(loc, sched[i].slot1400);
    loc++;
    EEPROM.update(loc, sched[i].slot1415);
    loc++;
    EEPROM.update(loc, sched[i].slot1430);
    loc++;
    EEPROM.update(loc, sched[i].slot1445);
    loc++;
    EEPROM.update(loc, sched[i].slot1500);
    loc++;
    EEPROM.update(loc, sched[i].slot1515);
    loc++;
    EEPROM.update(loc, sched[i].slot1530);
    loc++;
    EEPROM.update(loc, sched[i].slot1545);
    loc++;
    EEPROM.update(loc, sched[i].slot1600);
    loc++;
    EEPROM.update(loc, sched[i].slot1615);
    loc++;
    EEPROM.update(loc, sched[i].slot1630);
    loc++;
    EEPROM.update(loc, sched[i].slot1645);
    loc++;
    EEPROM.update(loc, sched[i].slot1700);
    loc++;
    EEPROM.update(loc, sched[i].slot1715);
    loc++;
    EEPROM.update(loc, sched[i].slot1730);
    loc++;
    EEPROM.update(loc, sched[i].slot1745);
    loc++;
    EEPROM.update(loc, sched[i].slot1800);
    loc++;
    EEPROM.update(loc, sched[i].slot1815);
    loc++;
    EEPROM.update(loc, sched[i].slot1830);
    loc++;
    EEPROM.update(loc, sched[i].slot1845);
    loc++;
    EEPROM.update(loc, sched[i].slot1900);
    loc++;
    EEPROM.update(loc, sched[i].slot1915);
    loc++;
    EEPROM.update(loc, sched[i].slot1930);
    loc++;
    EEPROM.update(loc, sched[i].slot1945);
    loc++;
    EEPROM.update(loc, sched[i].slot2000);
    loc++;
    EEPROM.update(loc, sched[i].slot2015);
    loc++;
    EEPROM.update(loc, sched[i].slot2030);
    loc++;
    EEPROM.update(loc, sched[i].slot2045);
    loc++;
    EEPROM.update(loc, sched[i].slot2100);
    loc++;
    EEPROM.update(loc, sched[i].slot2115);
    loc++;
    EEPROM.update(loc, sched[i].slot2130);
    loc++;
    EEPROM.update(loc, sched[i].slot2145);
    loc++;
    EEPROM.update(loc, sched[i].slot2200);
    loc++;
    EEPROM.update(loc, sched[i].slot2215);
    loc++;
    EEPROM.update(loc, sched[i].slot2230);
    loc++;
    EEPROM.update(loc, sched[i].slot2245);
    loc++;
    EEPROM.update(loc, sched[i].slot2300);
    loc++;
    EEPROM.update(loc, sched[i].slot2315);
    loc++;
    EEPROM.update(loc, sched[i].slot2330);
    loc++;
    EEPROM.update(loc, sched[i].slot2345);
    loc++;
  }
  SetScheduleFlag();
}
*/

void ReadSchedFromEEPROM() {
  unsigned int i = 0;
  unsigned int loc = 0;

  for(i = 0; i < 16; i++) {
    loc = 0 + (i * sizeof(struct schedule));
#ifdef _DEBUG
    Serial.print("ReadSchedFromEEPROM(): Block read starts at ");
    Serial.println(loc);
#endif
    EEPROM.get(loc, sched[i]);
  }
}

/*
void ReadSchedFromEEPROM() {
  unsigned int i = 0;
  unsigned int loc = 0;

  for(i = 0; i < 16; i++) {
    loc = 0 + (i * sizeof(struct schedule));
#ifdef _DEBUG
    Serial.print("ReadSchedFromEEPROM(): Block read starts at ");
    Serial.println(loc);
#endif    
    sched[i].slot0000 = EEPROM.read(loc);
    loc++;
    sched[i].slot0015 = EEPROM.read(loc);
    loc++;
    sched[i].slot0030 = EEPROM.read(loc);
    loc++;
    sched[i].slot0045 = EEPROM.read(loc);
    loc++;
    sched[i].slot0100 = EEPROM.read(loc);
    loc++;
    sched[i].slot0115 = EEPROM.read(loc);
    loc++;
    sched[i].slot0130 = EEPROM.read(loc);
    loc++;
    sched[i].slot0145 = EEPROM.read(loc);
    loc++;
    sched[i].slot0200 = EEPROM.read(loc);
    loc++;
    sched[i].slot0215 = EEPROM.read(loc);
    loc++;
    sched[i].slot0230 = EEPROM.read(loc);
    loc++;
    sched[i].slot0245 = EEPROM.read(loc);
    loc++;
    sched[i].slot0300 = EEPROM.read(loc);
    loc++;
    sched[i].slot0315 = EEPROM.read(loc);
    loc++;
    sched[i].slot0330 = EEPROM.read(loc);
    loc++;
    sched[i].slot0345 = EEPROM.read(loc);
    loc++;
    sched[i].slot0400 = EEPROM.read(loc);
    loc++;
    sched[i].slot0415 = EEPROM.read(loc);
    loc++;
    sched[i].slot0430 = EEPROM.read(loc);
    loc++;
    sched[i].slot0445 = EEPROM.read(loc);
    loc++;
    sched[i].slot0500 = EEPROM.read(loc);
    loc++;
    sched[i].slot0515 = EEPROM.read(loc);
    loc++;
    sched[i].slot0530 = EEPROM.read(loc);
    loc++;
    sched[i].slot0545 = EEPROM.read(loc);
    loc++;
    sched[i].slot0600 = EEPROM.read(loc);
    loc++;
    sched[i].slot0615 = EEPROM.read(loc);
    loc++;
    sched[i].slot0630 = EEPROM.read(loc);
    loc++;
    sched[i].slot0645 = EEPROM.read(loc);
    loc++;
    sched[i].slot0700 = EEPROM.read(loc);
    loc++;
    sched[i].slot0715 = EEPROM.read(loc);
    loc++;
    sched[i].slot0730 = EEPROM.read(loc);
    loc++;
    sched[i].slot0745 = EEPROM.read(loc);
    loc++;
    sched[i].slot0800 = EEPROM.read(loc);
    loc++;
    sched[i].slot0815 = EEPROM.read(loc);
    loc++;
    sched[i].slot0830 = EEPROM.read(loc);
    loc++;
    sched[i].slot0845 = EEPROM.read(loc);
    loc++;
    sched[i].slot0900 = EEPROM.read(loc);
    loc++;
    sched[i].slot0915 = EEPROM.read(loc);
    loc++;
    sched[i].slot0930 = EEPROM.read(loc);
    loc++;
    sched[i].slot0945 = EEPROM.read(loc);
    loc++;
    sched[i].slot1000 = EEPROM.read(loc);
    loc++;
    sched[i].slot1015 = EEPROM.read(loc);
    loc++;
    sched[i].slot1030 = EEPROM.read(loc);
    loc++;
    sched[i].slot1045 = EEPROM.read(loc);
    loc++;
    sched[i].slot1100 = EEPROM.read(loc);
    loc++;
    sched[i].slot1115 = EEPROM.read(loc);
    loc++;
    sched[i].slot1130 = EEPROM.read(loc);
    loc++;
    sched[i].slot1145 = EEPROM.read(loc);
    loc++;
    sched[i].slot1200 = EEPROM.read(loc);
    loc++;
    sched[i].slot1215 = EEPROM.read(loc);
    loc++;
    sched[i].slot1230 = EEPROM.read(loc);
    loc++;
    sched[i].slot1245 = EEPROM.read(loc);
    loc++;
    sched[i].slot1300 = EEPROM.read(loc);
    loc++;
    sched[i].slot1315 = EEPROM.read(loc);
    loc++;
    sched[i].slot1330 = EEPROM.read(loc);
    loc++;
    sched[i].slot1345 = EEPROM.read(loc);
    loc++;
    sched[i].slot1400 = EEPROM.read(loc);
    loc++;
    sched[i].slot1415 = EEPROM.read(loc);
    loc++;
    sched[i].slot1430 = EEPROM.read(loc);
    loc++;
    sched[i].slot1445 = EEPROM.read(loc);
    loc++;
    sched[i].slot1500 = EEPROM.read(loc);
    loc++;
    sched[i].slot1515 = EEPROM.read(loc);
    loc++;
    sched[i].slot1530 = EEPROM.read(loc);
    loc++;
    sched[i].slot1545 = EEPROM.read(loc);
    loc++;
    sched[i].slot1600 = EEPROM.read(loc);
    loc++;
    sched[i].slot1615 = EEPROM.read(loc);
    loc++;
    sched[i].slot1630 = EEPROM.read(loc);
    loc++;
    sched[i].slot1645 = EEPROM.read(loc);
    loc++;
    sched[i].slot1700 = EEPROM.read(loc);
    loc++;
    sched[i].slot1715 = EEPROM.read(loc);
    loc++;
    sched[i].slot1730 = EEPROM.read(loc);
    loc++;
    sched[i].slot1745 = EEPROM.read(loc);
    loc++;
    sched[i].slot1800 = EEPROM.read(loc);
    loc++;
    sched[i].slot1815 = EEPROM.read(loc);
    loc++;
    sched[i].slot1830 = EEPROM.read(loc);
    loc++;
    sched[i].slot1845 = EEPROM.read(loc);
    loc++;
    sched[i].slot1900 = EEPROM.read(loc);
    loc++;
    sched[i].slot1915 = EEPROM.read(loc);
    loc++;
    sched[i].slot1930 = EEPROM.read(loc);
    loc++;
    sched[i].slot1945 = EEPROM.read(loc);
    loc++;
    sched[i].slot2000 = EEPROM.read(loc);
    loc++;
    sched[i].slot2015 = EEPROM.read(loc);
    loc++;
    sched[i].slot2030 = EEPROM.read(loc);
    loc++;
    sched[i].slot2045 = EEPROM.read(loc);
    loc++;
    sched[i].slot2100 = EEPROM.read(loc);
    loc++;
    sched[i].slot2115 = EEPROM.read(loc);
    loc++;
    sched[i].slot2130 = EEPROM.read(loc);
    loc++;
    sched[i].slot2145 = EEPROM.read(loc);
    loc++;
    sched[i].slot2200 = EEPROM.read(loc);
    loc++;
    sched[i].slot2215 = EEPROM.read(loc);
    loc++;
    sched[i].slot2230 = EEPROM.read(loc);
    loc++;
    sched[i].slot2245 = EEPROM.read(loc);
    loc++;
    sched[i].slot2300 = EEPROM.read(loc);
    loc++;
    sched[i].slot2315 = EEPROM.read(loc);
    loc++;
    sched[i].slot2330 = EEPROM.read(loc);
    loc++;
    sched[i].slot2345 = EEPROM.read(loc);
    loc++;
  }
}
*/

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
