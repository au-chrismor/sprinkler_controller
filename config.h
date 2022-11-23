#define _DEBUG
#define _NET_ETHERNET

#ifdef _NET_ETHERNET
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};
unsigned int localPort = 8888;
const int NTP_PACKET_SIZE = 48;
#define ETHERNET_CS 10
#endif


#define MQTT_HOST "54.253.81.77"
#define MY_ID "673bfa34-09f6-4f46-83c7-d745c5f1cbd8"
#define MY_PASS "N0tagain"
#define NTP_HOST "time.windows.com"

#define SUN   0x01
#define MON   0x02
#define TUE   0x04
#define WED   0x08
#define THU   0x10
#define FRI   0x20
#define SAT   0x40
#define DAILY SUN | MON | TUE | WED | THU | FRI | SAT

#define RLY00 22
#define RLY01 24
#define RLY02 26
#define RLY03 28
#define RLY04 30
#define RLY05 31
#define RLY06 32
#define RLY07 33
#define RLY08 34
#define RLY09 34
#define RLY10 35
#define RLY11 36
#define RLY12 37
#define RLY13 38
#define RLY14 39
#define RLY15 40
#define RELAY_OFF HIGH
#define RELAY_ON  LOW
