#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

#define echoPin 4  // GPIO4 (D2 on NodeMCU)
#define trigPin 2  // GPIO2 (D4 on NodeMCU)
#define buzzerPin D0

const char* ssid = "WIFI";
const char* password = "PASSWORD";
const char* badRequest = "https://rateto-backend.onrender.com/ESP8266";
const char* goodRequest = "https://rateto-backend.onrender.com/ESP8266_GOOD";

long duration = 0;
int distance = 0;
unsigned long count = 0; 
unsigned long lastPositiveRequestTime = 0;

void setup() {
  Serial.begin(9600);
  delay(10);

  Serial.println("\nBooting up...");
  Serial.println(ESP.getResetReason());

  pinMode(buzzerPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);  
  digitalWrite(buzzerPin, LOW); 

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi. Restarting...");
    ESP.restart();
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  unsigned long currentTime = millis();
  if (currentTime - lastPositiveRequestTime > 1800000) {
    positiveRequest();
    lastPositiveRequestTime = currentTime;
  }

  distance = reader();
  Serial.print("Distance: ");
  Serial.println(distance);

  if (distance <= 0 || distance >= 1000) {
    unfoundState();
  }

  delay(1000);
}

int reader() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) {
    return -1;
  }

  return duration / 58.2; 
}

void buzzer(int state) {
  digitalWrite(buzzerPin, state ? HIGH : LOW);
}

void unfoundState() {
  Serial.println("UNFOUND STATE");

  for (int i = 0; i < 20; i++) {

    distance = reader();
    Serial.print("Distance: ");
    Serial.println(distance);

    if (distance > 0 && distance < 10) {
      Serial.println("Object found again. Exiting unfoundState.");
      return;  
    }

    buzzer(HIGH);
    delay(200); 
    buzzer(LOW);
    delay(800); 
  }

  Serial.println("TOO LATE: Sending negative request.");
  negativeRequest();
}

void positiveRequest() {
  sendHttpRequest(goodRequest);
}

void negativeRequest() {
  sendHttpRequest(badRequest);
}

void sendHttpRequest(const char* url) {
  WiFiClientSecure client;
  client.setInsecure();  // For testing, use a proper certificate in production

  HTTPClient http;
  http.setTimeout(5000);  // 5-second timeout

  Serial.print("Sending request to ");
  Serial.println(url);

  if (http.begin(client, url)) {
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("HTTP GET successful. Code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Response:");
      Serial.println(payload);
    } else {
      Serial.printf("HTTP GET failed. Code: %d\n", httpCode);
    }

    http.end();
  } else {
    Serial.println("Failed to initialize HTTP connection.");
  }
}
