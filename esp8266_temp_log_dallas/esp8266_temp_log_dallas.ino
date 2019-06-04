// Import required libraries
#include "ESP8266WiFi.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

//#define DEBUG

#include "config.h"
IPAddress ip(192,168,1,151);
IPAddress dns(192,168,1,1);
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);   

WiFiClient wificlient;
PubSubClient client(wificlient);

#define ONE_WIRE_BUS D6
#define ONE_WIRE_POWER D7
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

#define WIFI_ENABLE_MAGIC 0xF0F0
#define WIFI_ENABLE_OFFSET 0

uint32_t wifi_enable;

#define LAST_MEASUREMENT_OFFSET 1

#define SLEEP_TIME 3600000000

typedef struct {
  uint32_t bat;
  int temp;
} rtcData;

void setup() {  
  ESP.rtcUserMemoryRead(WIFI_ENABLE_OFFSET, (uint32_t*) &wifi_enable, sizeof(wifi_enable));

  switch(wifi_enable)
  {
    // wifi mode
    case WIFI_ENABLE_MAGIC:
    {  
      #ifdef DEBUG
        // Start Serial
        Serial.begin(115200);
        delay(10);
        Serial.println();
        Serial.println( "WIFI mode" );        
        Serial.print("Connecting to ");
        Serial.println(ssid); 
      #endif

      // We start by connecting to a WiFi network
      WiFi.config(ip, gateway, subnet, dns);    
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(100);
      }
    
      #ifdef DEBUG
        Serial.println();
        Serial.print("Wifi connected: ");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      #endif
    
      client.setServer(mqtthost, 1883);
    
      while (!client.connected()) {
        if ( client.connect("esp8266", mqttuser, mqttpass) ) {
          #ifdef DEBUG
            Serial.println( "mqtt connected [DONE]" );
          #endif
        } else {
          #ifdef DEBUG
            Serial.print( "[FAILED] [ rc = " );
            Serial.print( client.state() );
            Serial.println( " : retrying in 5 seconds]" );
          #endif
          // Wait 5 seconds before retrying
          delay( 5000 );
        }
      }
      
      rtcData rtcdata;
      ESP.rtcUserMemoryRead(LAST_MEASUREMENT_OFFSET, (uint32_t*) &rtcdata, sizeof(rtcdata));
         
      String temperature = String(rtcdata.temp);
      String batlevel = String(rtcdata.bat);
      
      // Prepare an influxdb string
      String payload = "espsensor ";
      payload += "temperature="; payload += temperature;
      payload += ",";
      payload += "battery="; payload += batlevel;
    
      // Send payload
      char attributes[100];
      payload.toCharArray( attributes, 100 );
      client.publish( topic, attributes );
      #ifdef DEBUG
        Serial.println( attributes );
      #endif
        
      // make sure message sent
      wificlient.flush();

      wifi_enable = 0;
      ESP.rtcUserMemoryWrite(WIFI_ENABLE_OFFSET, (uint32_t*) &wifi_enable, sizeof(wifi_enable));
      ESP.deepSleep(SLEEP_TIME, WAKE_RF_DISABLED);
      break;
    }
    // measurement mode
    default:
    {
      #ifdef DEBUG
        // Start Serial
        Serial.begin(115200);      
        Serial.println();
        Serial.println( "MEASURE mode" );
      #endif
      
      pinMode(A0, INPUT);      
      pinMode(ONE_WIRE_POWER, OUTPUT);
      digitalWrite(ONE_WIRE_POWER, HIGH);
      sensors.begin();
      sensors.requestTemperatures(); 
      
      int temp;    
      temp = sensors.getTempCByIndex(0);
      
      // shut sensor immediately
      pinMode(ONE_WIRE_POWER, INPUT);

      #ifdef DEBUG
        Serial.print( "Temperature : " );
        Serial.println( temp, DEC );
      #endif

      uint32_t bat = analogRead(A0);
      // Empirical calibration
      bat = bat*3700/720;
      #ifdef DEBUG
        Serial.print("Battery mV : ");
        Serial.println(bat);    
      #endif

      rtcData rtcdata;
      rtcdata.bat = bat;
      rtcdata.temp = temp;
      
      ESP.rtcUserMemoryWrite(LAST_MEASUREMENT_OFFSET, (uint32_t*) &rtcdata, sizeof(rtcdata));
            
      wifi_enable = WIFI_ENABLE_MAGIC;
      ESP.rtcUserMemoryWrite(WIFI_ENABLE_OFFSET, (uint32_t*) &wifi_enable, sizeof(wifi_enable));     
      ESP.deepSleep(1000000, WAKE_RF_DEFAULT);
      break;
    }
  }
}

void loop() {
}
