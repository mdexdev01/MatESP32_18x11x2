// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://www.shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

/*
  TODO
  Store ssid, pass,  
  Store my last ip
  Get Static IP from AP
  
*/

#include <WiFi.h>
#include <MQTT.h>

const char* ssid     = "KT_GiGA_2G_Wave2_4587"; // Change this to your WiFi SSID
const char* pass = "fab8ezx455"; // Change this to your WiFi password
// const char ssid[] = "ssid";
// const char pass[] = "pass";
IPAddress local_ip;                    // the IP address of your shield
int tcp_server_port = 888;
WiFiServer server(tcp_server_port);


WiFiClient net;
MQTTClient mqttClient;
char * mqtt_topic_server = "/xedm01/grib/server";
char * mqtt_topic_username = "/xedm01/grib/user";


unsigned long lastMillis = 0;

void mqttConnect() {
  Serial.print("mqtt) checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nmqtt) try mqtt connecting...");
  while (!mqttClient.connect("arduino", "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nmqtt) connected!");

   mqttClient.subscribe(mqtt_topic_server);
   mqttClient.subscribe(mqtt_topic_username);
 // mqttClient.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the mqttClient in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `mqttClient.loop()`.
}



void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

	while(WiFi.status() != WL_CONNECTED) {
		delay(100);
	}
  Serial.printf("AP access done....\n");

  //  CHECK MY LOCAL IP
  local_ip = WiFi.localIP();
  Serial.print("my local ip is :");
  Serial.println(local_ip);

  char txt_buf[32];
  sprintf(txt_buf, "ip:%s", local_ip.toString().c_str());
  Serial.println(txt_buf);

  // LAUNCH SERVER...
  setup_tcp_server();
  
  //  CONNECT MQTT CLIENT
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  mqttClient.begin("public.cloud.shiftr.io", net);
  mqttClient.onMessage(messageReceived);

  mqttConnect();
}

void setup_tcp_server() {
    server.begin();
}


int loop_count = 0;
int tx_count = 0;

WiFiClient client;

void loop() {
  // delay(100);

  //  IF NO CLIENT, LISTEN. AND TURN ON MQTT
  if(client.connected() == false) {
    client = server.available();   // listen for incoming clients

    if(client.connected() == true) {
      Serial.println("New Client.");           // print a message out the serial port
    }
    else {
      loop_mqtt();  
      return;
    }
  }

  //  RX  & TX
  if (client.available()) {            // loop while the client's connected
    //  RX    
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    //  ACK TX serial
    {
      // char msg_buf[32];
      // sprintf(msg_buf, "[ack]tcp server msg %d\n", loop_count);
      // client.println(msg_buf);
      // Serial.println(msg_buf);
    }   
  }

  //  TX binary
  {

    int packet_len = 512;
    uint8_t packet_buf[packet_len + 1];
    memset(packet_buf, 90, packet_len);      
    packet_buf[0] = 0xFF;
    packet_buf[1] = 0xFD;
    packet_buf[2] = byte(tx_count / 10000) + 1;
    packet_buf[3] = byte((tx_count % 10000) / 100) + 1;
    packet_buf[4] = byte(tx_count % 100) + 1;
    packet_buf[packet_len-2] = 0xFD;
    packet_buf[packet_len-1] = 0xFE;
    
    packet_buf[packet_len] = 0x00;
    int ret_val = client.write((char *)packet_buf);

    Serial.printf("[%8d]tx %d bytes \n", tx_count, ret_val);

    tx_count++;
    
    delay(20);
  }

  mqttClient.loop();
  loop_count++;
}

void loop_mqtt() {
  mqttClient.loop();

  if (!mqttClient.connected()) {
    mqttConnect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 2000) {
    lastMillis = millis();
    
    char msg_buf[80];
    //"[server]ip:172.3.5.87, port:380\r\n";
    sprintf(msg_buf, "[server]ip:%s, port:%d, val12:d4dFo2zVkj2\r\n", local_ip.toString(), tcp_server_port);
    // mqttClient.publish("/xedm01", msg_buf);
    mqttClient.publish(mqtt_topic_server, msg_buf);
    // Serial.println(msg_buf);
  }

}
