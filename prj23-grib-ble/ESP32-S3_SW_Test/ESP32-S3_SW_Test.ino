#define LED_R 14
#define LED_G 21
#define LED_B 47
#define SW_1 45
#define SW_2 48
#define SW_DIP 12

void setup() {
  // put your setup code here, to run once:
  pinMode(SW_1, INPUT);
  pinMode(SW_2, INPUT);
  pinMode(SW_DIP, INPUT);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);

  Serial.begin(115200);
  Serial1.begin(115200);
  //Serial.begin(921600);
  //Serial1.begin(921600, SERIAL_8N1, 18, 17);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("\nSW Testing..."); //display a message 
  int switch_1 = digitalRead(SW_1);
  if (switch_1 == LOW) {
    digitalWrite(LED_R, LOW); // LED 켜기
    Serial.println("SW_1 ON, LED_R 켜짐");
    Serial.println("\n -- rebooting -- ");
    ESP.restart();

  } else {
    digitalWrite(LED_R, HIGH); // LED 끄기
    Serial.println("SW_1 OFF, LED_R 꺼짐");
  }

  int switch_2 = digitalRead(SW_2);
  if (switch_2 == LOW) {
    digitalWrite(LED_G, LOW); // LED 켜기
    Serial.println("SW_2 ON, LED_G 켜짐");
  } else {
    digitalWrite(LED_G, HIGH); // LED 끄기
    Serial.println("SW_2 OFF, LED_G 꺼짐");
  }
  
  int switch_dip = digitalRead(SW_DIP);
  if (switch_dip == LOW) {
    digitalWrite(LED_B, LOW); // LED 켜기
    Serial.println("SW_DIP ON, LED_B 켜짐");
  } else {
    digitalWrite(LED_B, HIGH); // LED 끄기
    Serial.println("SW_DIP OFF, LED_B 꺼짐");
  }

/*  
  if(Serial1.available()){ 
    Serial.write(Serial1.read());
  }

  Serial1.write(c);
*/
  delay(1000);
}
