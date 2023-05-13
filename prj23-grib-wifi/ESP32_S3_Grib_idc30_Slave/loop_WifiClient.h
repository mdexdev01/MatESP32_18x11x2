#include <WiFi.h>

const char* ssid     = "KT_GiGA_2G_Wave2_4587"; // Change this to your WiFi SSID
const char* password = "fab8ezx455"; // Change this to your WiFi password


void scan_AP();
void txrxClient();
void loop_WifiClient();

void set_WIFIClient() {
  WiFi.mode(WIFI_STA);
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

  loop_WifiClient();
}


char *      host = "172.30.1.70";
uint16_t    port = 10003;
uint32_t    try_count = 0;

void loop_WifiClient() {
  WiFiClient client;

  Serial.printf("try connecting...%s, %d \n", host, port);

  while(true) {    
    if(!client.connect(host, port)) {
      Serial.printf("connect failed %d \n", try_count);
      delay(1000);
    } 
    else {
      Serial.println("GOOOOD!");
      client.print("Hello from ESP32");
      break;
    }

    try_count++;
  }

  while(true) {
    Serial.println("Ends... ");
  }

}


void scan_AP() {
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          delay(10);
      }
  }
  Serial.println("");

  // Wait a bit before scanning again
  delay(5000);

}
