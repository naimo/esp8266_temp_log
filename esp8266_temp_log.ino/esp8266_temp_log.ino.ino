// Import required libraries
#include "ESP8266WiFi.h"
#include "DHTesp.h"
#include <PubSubClient.h>

#include "config.h"

WiFiClient wificlient;
PubSubClient client(wificlient);

DHTesp dht;

void setup() {
  
  // Start Serial
  Serial.begin(115200);
  delay(10);
 
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtthost, 1883 );
  
  dht.setup(6); // data pin 2
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to reconnect");
    WiFi.begin(ssid, password);
    delay(500);
  }
  
  if ( !client.connected() ) {
    reconnect();
  }
   
  // Reading temperature and humidity
  float h = dht.getHumidity();
  // Read temperature as Celsius
  float t = dht.getTemperature();

  String temperature = String(t);
  String humidity = String(h);

  // Just debug messages
  Serial.print( "Sending temperature and humidity : [" );
  Serial.print( temperature ); Serial.print( "," );
  Serial.print( humidity );
  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity;
  payload += "}";

  // Send payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "v1/devices/me/telemetry", attributes );
  Serial.println( attributes );
  
  delay(10000);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Connecting ...");
    if ( client.connect("esp8266",mqttuser,mqttpass) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}
