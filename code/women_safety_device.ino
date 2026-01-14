#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// Create GPS and GSM serials
HardwareSerial gpsSerial(2);   // UART2 for GPS
HardwareSerial sim800(1);      // UART1 for GSM
TinyGPSPlus gps;

// Define pins
#define GPS_RX 4       // GPS TX → ESP32 RX (GPIO4)
#define GPS_TX -1      // Not used
#define GSM_RX 16      // SIM800L TX → ESP32 RX16
#define GSM_TX 17      // SIM800L RX ← ESP32 TX17
#define BUTTON_PIN 15  // Push button to GND

// Recipient phone number
const char PHONE_NUMBER[] = "+917851059980";

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  sim800.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Initializing GSM module...");
  delay(3000);

  sendAT("AT");
  sendAT("ATE0");              // Disable echo
  sendAT("AT+CMGF=1");         // SMS text mode
  sendAT("AT+CSCS=\"GSM\"");   // GSM charset
  sendAT("AT+CSMP=17,167,0,0");// standard SMS params

  Serial.println("System ready. Press button to send location SMS...");
}

void loop() {
  static bool buttonPressed = false;
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && !buttonPressed) {
    delay(50); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      buttonPressed = true;
      Serial.println("Button pressed!");
      handleButtonPress();
    }
  }

  if (currentState == HIGH && buttonPressed) {
    buttonPressed = false;
  }
}

// ---------------------- Core Function ------------------------
void handleButtonPress() {
  Serial.println("Waiting for GPS fix...");
  unsigned long start = millis();
  bool gotFix = false;

  while (millis() - start < 30000) { // wait up to 30 sec
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
    if (gps.location.isValid()) {
      gotFix = true;
      break;
    }
  }

  if (gotFix) {
    double lat = gps.location.lat();
    double lon = gps.location.lng();

    Serial.println("GPS Fix Acquired!");
    Serial.print("Latitude : "); Serial.println(lat, 6);
    Serial.print("Longitude: "); Serial.println(lon, 6);

    // ONE-LINE, PLAIN ASCII MESSAGE
    String msg = "SOS LAT: ";
    msg += String(lat, 6);
    msg += " LON: ";
    msg += String(lon, 6);

    sendSMS(PHONE_NUMBER, msg);
    Serial.println("SMS sent with location.");

    // optional call
    callNumber(PHONE_NUMBER);
  } else {
    Serial.println("No GPS fix found! Sending warning SMS...");
    sendSMS(PHONE_NUMBER, "SOS: GPS FIX NOT FOUND");
    callNumber(PHONE_NUMBER);
  }
}

// ---------------------- Helper Functions ------------------------
void sendAT(String cmd) {
  sim800.println(cmd);
  delay(200);
  while (sim800.available()) Serial.write(sim800.read());
}

void sendSMS(String number, String text) {
  sim800.println("AT+CMGF=1");
  delay(200);
  sim800.println("AT+CSCS=\"GSM\"");
  delay(200);
  sim800.println("AT+CSMP=17,167,0,0");
  delay(200);

  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(500);

  sim800.print(text);   // plain text, no \n
  sim800.write(26);     // CTRL+Z
  delay(3000);
  Serial.println("SMS command sent.");
}

void callNumber(String number) {
  Serial.print("Calling ");
  Serial.println(number);

  sim800.print("ATD");
  sim800.print(number);
  sim800.println(";");
  delay(20000); // ring 20 s

  sim800.println("ATH");
  delay(1000);
  Serial.println("Call ended.");
}
