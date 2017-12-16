const int ledPin            = 4;
const int buzzerPin         = 5;
const int inPin             = 2;
int val = 0;  


void setup() {
  pinMode(ledPin, OUTPUT);    
  pinMode(buzzerPin, OUTPUT); 
  pinMode(inPin, INPUT); 
  activation();

}

// the loop function runs over and over again forever
void loop() {
  val = digitalRead(inPin);     // read the input pin
  if (val == HIGH) {
    digitalWrite(ledPin, HIGH); 
   alarm();
  }
  else digitalWrite(ledPin, LOW); 
}


void alarm() {
  // Whoop up
  for(int hz = 800; hz < 1500; hz++){
    tone(buzzerPin, hz, 50);
    delay(2);
  }
  noTone(buzzerPin);
} // Repeat


void activation() {
 for(int i = 0; i < 10; i++){
    digitalWrite(ledPin, HIGH); 
    delay(500);
    digitalWrite(ledPin, LOW); 
    delay(500);
  } 
  for(int i = 0; i < 50; i++){
    digitalWrite(ledPin, HIGH); 
    delay(100);
    digitalWrite(ledPin, LOW); 
    delay(50);
  }
  digitalWrite(ledPin, HIGH); 
  delay(3000);
  digitalWrite(ledPin, LOW); 
}
