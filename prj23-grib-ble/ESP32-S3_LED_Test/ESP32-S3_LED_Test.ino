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
  Serial.println("LED Testing..."); //display a message 

  digitalWrite(LED_R, LOW);
  delay(500);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, LOW);
  delay(500);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);
  delay(500);
  digitalWrite(LED_B, HIGH);

/*  
  if(Serial1.available()){ 
    Serial.write(Serial1.read());
  }

  Serial1.write(c);
*/
  delay(1000);
}
