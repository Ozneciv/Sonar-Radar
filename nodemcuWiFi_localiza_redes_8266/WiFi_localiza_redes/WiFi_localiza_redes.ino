#include "ESP8266WiFi.h" // biblioteca para NodeMCU ESP8266 trabalhar com WiFi
//#include "WiFi.h" // biblioteca para ESP32 trabalhar com WiFi

void setup() {
Serial.begin(9600);
// Set WiFi to station mode and disconnect from an AP if it was previously connected
WiFi.mode(WIFI_STA);
WiFi.disconnect();
delay(2000);
Serial.println("Setup done");
}

// ------ Localiza redes ----------
void loop() {
Serial.println("scan start");
int n = WiFi.scanNetworks();// WiFi.scanNetworks will return the number of networks found
Serial.println("scan done");
if (n == 0)
Serial.println("no networks found");
else
{
Serial.print(n);
Serial.println(" networks found");
for (int i = 0; i < n; ++i)
{
// Print SSID and RSSI for each network found
//SSID = Service Set Identifier
//RSSI = Received Signal Strength Indication
Serial.print(i + 1);
Serial.print(": ");
Serial.print(WiFi.SSID(i));
Serial.print(" (");
Serial.print(WiFi.RSSI(i));
Serial.print(")");
Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
delay(10);
}
}
Serial.println("");
// Wait a bit before scanning again
delay(5000);
}
