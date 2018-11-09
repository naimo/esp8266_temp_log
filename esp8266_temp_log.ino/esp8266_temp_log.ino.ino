// Import required libraries
#include "ESP8266WiFi.h"
#include "DHTesp.h"
#include <PubSubClient.h>

//#define DEBUG

#include "config.h"
IPAddress ip(192,168,1,151);
IPAddress dns(192,168,1,1);
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);   

WiFiClient wificlient;
PubSubClient client(wificlient);

DHTesp dht;

#define WIFI_ENABLE_MAGIC 0xF0F0
#define WIFI_ENABLE_OFFSET 0

uint32_t wifi_enable;

#define LAST_MEASUREMENT_OFFSET 1

typedef struct {
  uint32_t bat;
  TempAndHumidity dhtdata;
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
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid); 
      #endif

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
         
      String temperature = String(rtcdata.dhtdata.temperature);
      String humidity = String(rtcdata.dhtdata.humidity);
      String batlevel = String(rtcdata.bat);
      
      // Prepare a JSON payload string
      String payload = "{";
      payload += "\"temperature\":"; payload += temperature;
      payload += ",";
      payload += "\"humidity\":"; payload += humidity;
      payload += ",";
      payload += "\"battery\":"; payload += batlevel;
      payload += "}";
    
      // Send payload
      char attributes[100];
      payload.toCharArray( attributes, 100 );
      client.publish( "v1/devices/me/telemetry", attributes );
      #ifdef DEBUG
        Serial.println( attributes );
      #endif
        
      // make sure message sent
      wificlient.flush();

      wifi_enable = 0;
      ESP.rtcUserMemoryWrite(WIFI_ENABLE_OFFSET, (uint32_t*) &wifi_enable, sizeof(wifi_enable));
      ESP.deepSleep(10 * 300000, WAKE_RF_DISABLED);
    }
    // measurement mode
    default:
    {
      pinMode(A0, INPUT);      
      pinMode(D5, OUTPUT);
      digitalWrite(D5, HIGH);
      dht.setup(D6, DHTesp::DHT11); // D6

      #ifdef DEBUG
        // Start Serial
        Serial.begin(115200);
        delay(1000);
      #endif
      
      TempAndHumidity values;    
      // throw away first values;
      values = dht.getTempAndHumidity();
    
      #ifdef DEBUG
        Serial.println();
        Serial.print( "Thrown temperature and humidity : [" );
        Serial.print( values.temperature, DEC );
        Serial.print( "," );
        Serial.print( values.humidity, DEC );
        Serial.println( "]" );
      #endif
      
      delay(2000);
         
      // Reading temperature and humidity
      values = dht.getTempAndHumidity();
    
      // shut dht immediately
      pinMode(D5, INPUT);

      #ifdef DEBUG
        Serial.print( "Kept temperature and humidity : [" );
        Serial.print( values.temperature, DEC );
        Serial.print( "," );
        Serial.print( values.humidity, DEC );
        Serial.println( "]" );
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
      rtcdata.dhtdata = values;
      
      ESP.rtcUserMemoryWrite(LAST_MEASUREMENT_OFFSET, (uint32_t*) &rtcdata, sizeof(rtcdata));
            
      wifi_enable = WIFI_ENABLE_MAGIC;
      ESP.rtcUserMemoryWrite(WIFI_ENABLE_OFFSET, (uint32_t*) &wifi_enable, sizeof(wifi_enable));     
      ESP.deepSleep(10 * 300000, WAKE_RF_DEFAULT);
    }
  }
}

void loop() {
}
