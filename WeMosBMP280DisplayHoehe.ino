#include <Servo.h>
#include <Adafruit_SSD1306.h>
#include <BMP280_DEV.h> 
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WebServer.h>  // <WebServer.h> for ESP32  

/*************************************************** 
  Dieses Programm ermittelt mit dem BMP280 Sensor und einem Wemos D1 (ESP8266) die maximal erreichte 
  Flughöhe und zeigt diesen Wert auf einem OLED-Display (SSD1306) an. Messintervall 50 ms --> 20 Messungen pro Sekunde.
  BME280 und SSD1306 sind per I2C (Wemos Pin D2 --> SDA, Pin D1 --> SCL) am Wemos D1 angeschlossen.
  Zum Einstellen der Starthöhe Null ist ein Taster am Wemos angeschlossen. (3,3 V zum Taster Eingang, Ausgang an 
  Pin D7 und D8 und über einen 10KOhm Widerstand mit GND verbunden. Um den Fallschirm manuell auzulösen ist ein weiterer Taster an D8 angeschlossen.
  Der Fallschirm löst nach erreichen der Maximalhöhe nach 5 m (hdiff) automatisch aus.
        3,3 V
        |
        |
       Button     
        |––––––– D7 (Reset)/D8 (Fallschirm)
        |
       10KOhm 
        |
        |
        GND

   Ein Servo an Pin D6 löst x m unter der maximalen Flughöhe (oder beim Druck auf Button 2) den Fallschirm aus.


  Spannungsüberwachung:
      Batterie
         |___________5V (WEMOS)
         |
         330KOhm
         |
         |__________ A0
         |
         |
         330KOhm
         |
         |
         GND 

  LEDs:

      D5 (grün) / D0 (rot)
              |
              |
              |
              220 Ohm
              |
              |
              LED
              |
              |
              GND

         
 ****************************************************/
// Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
// Connect GND to Ground
// Connect SCL to i2c clock D1
// Connect SDA to i2c data  D2
float temperature, pressure, altitude;            // Create the temperature, pressure and altitude variables
BMP280_DEV bmp280; 

// Connect VCC of the SSD1306 to 3.3V (NOT 5.0V!)
// Connect GND to Ground
// Connect SCL to i2c clock D1
// Connect SDA to i2c data  D2
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
//Variablen und Pins
float batterie;
const int button1Pin = 13;     // the number of the pushbutton pin D7
const int button2Pin = 15;     // the number of the pushbutton pin D8
int button1State = 0;           // beide Taster auf Null setzen
int button2State = 0; 
float h = 0; //aktuell gemessene Höhe
float h0 = 0; // gemessene Höhe beim Start/Taster gedrückt als Starthöhe
float hmax = 0; // maximal gemessene Höhe zur Anzeige auf dem Display
float hdiff = 5; //Höhendifferenz bei der der Fallschirm nach dem Erreichen der Maximalhöhe auslöst, default 5 m
int led[2] = {0,2}; //KontrollLEDs an D3 und D4 um HTML-Eingaben zu signalisieren (optional)
int ledgreen = 14; //grüne LED
int ledred = 16;  // rote LED
int servopin =12; //servo an D6
bool led_status[2] = {false}; //beide KontrollLEDs ausschalten
Servo MyServo;
//wlan
const char* ssid = "Wasserrakete1";  // Name des Wlan
const char* pass = "12345678"; // Passwort wlan
IPAddress ip(192,168,4,1); // should be 192.168.4.x
IPAddress gateway(192,168,4,1);  // should be 192.168.4.x
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80);  // WebServer server(80);
String headAndTitle = "<head><style>"
                        ".button {"
                          "border: none;"
                          "color: white;"
                          "width: 600px;"
                          "padding: 20px;"
                          "text-align: center;"
                          "margin: 20px 200px;"
                        "}"
                        ".greenButton {background-color: green; font-size: 64px;}"
                        ".redButton {background-color: red; font-size: 64px;}"
                        ".blueButton {background-color: blue; font-size: 50px;}"
                      "</style>"
                      "</head><meta http-equiv=\"refresh\" content=\"1\"></head>"
                      "</BR></BR><h1 align=\"center\">Wasserrakete 1</h1></div>"
                      "<div align=\"center\">Flugh&oumlhe messen und Fallschirm steuern</BR></div>";
String led0_1= "<a href=\"/led0_on\"><button class=\"button greenButton\">Reset on</button></a>";
String led0_0= "<a href=\"/led0_off\"><button class=\"button redButton\">Reset off</button></a>";
String led1_1= "</BR><a href=\"/led1_on\"><button class=\"button greenButton\">Fallschirm ausl&oumlsen</button></a>";
String led1_0= "</BR><a href=\"/led1_off\"><button class=\"button redButton\">Fallschirm r&uumlcksetzen</button></a>";




  
void setup() {
   Serial.begin(115200);
    Wire.begin();
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // die beiden Taster-Anschlüsse D7 und D8 als Eingang definieren:
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  //LED-Pins definieren
  pinMode(led[0], OUTPUT); //LED-Pin D3 als Ausgang definieren
  digitalWrite(led[0], LOW);//LED ausschalten
  pinMode(led[1], OUTPUT); //LED-Pin D4 als Ausgang definieren
  digitalWrite(led[1], LOW); //LED ausschalten
  pinMode(ledgreen, OUTPUT); //LED-Pin D5 als Ausgang definieren
  digitalWrite(ledgreen, LOW);//LED ausschalten
  pinMode(ledred, OUTPUT); //LED-Pin D0 als Ausgang definieren
  digitalWrite(ledred, LOW); //LED ausschalten

 
  // Clear the buffer
  display.clearDisplay();

  // text display Starttext
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(F("Flugh\x99he")); 
  display.setTextSize(1);
  display.setCursor(0,20);
  display.println("Markus Weber");
  display.display();
  
   MyServo.attach(servopin);  //Servo an D6 verbinden und auf Null stellen
   MyServo.write(0);
   delay(500);
   MyServo.detach(); //Servo entkoppeln um Strom zu sparen
  
   bmp280.begin(BMP280_I2C_ALT_ADDR); //BMP initialisieren
  // bmp280.setPresOversampling(OVERSAMPLING_X4);    // Set the pressure oversampling to X4
  // bmp280.setTempOversampling(OVERSAMPLING_X1);    // Set the temperature oversampling to X1
  // bmp280.setIIRFilter(IIR_FILTER_4);              // Set the IIR filter to setting 4
   bmp280.startNormalConversion(); 
       delay(2000);    
  if (bmp280.getMeasurements(temperature, pressure, altitude))    // Check if the measurement is complete
  {
    Serial.print(temperature);                    // Die Messergebnisse auf dem seriellen Monitor ausgeben 
    Serial.print(F("*C   "));
    Serial.print(pressure);    
    Serial.print(F("hPa   "));
    Serial.print(altitude);
    Serial.println(F("m"));  
  }
  else Serial.print("Fehler BMP-Sensor");
  


    
  display.clearDisplay();
  display.display();


  
  WiFi.softAPConfig(ip, gateway, subnet); //AP starten
  WiFi.softAP(ssid, pass);  
 
  delay(500); 
  server.on("/",handleRoot); //die HTML Eingaben definieren
  server.on("/led0_on", led0on);
  server.on("/led0_off", led0off);
  server.on("/led1_on", led1on);
  server.on("/led1_off", led1off);

  server.begin(); //den webserver starten

  h0 = altitude;// Starthöhe ermitteln und als h0 speichern
  
}
  
void loop() {


 if(bmp280.getMeasurements(temperature, pressure, altitude));
      { // read the state of the pushbutton value:
          button1State = digitalRead(button1Pin);
          button2State = digitalRead(button2Pin);
      // überprüfen ob pushbutton1 gedrückt ist oder LED0 über HTML gesetzt. h0 neu speichern und Flughöhe zurücksetzen Servo auf Null stellen
           if (button1State == HIGH or led_status[0] == true) {
              // Höhe auf Null setzen:
             h0 = altitude;
             hmax = 0;
            MyServo.attach(servopin);
            MyServo.write(0);
            delay(500);
            MyServo.detach();
             
        }

      //Servo
      //überprüfen ob pushbutton2 oder LED1 über HTML gesetzt, dann Servo bewegen um den Fallschirm auszulösen
          if (button2State == HIGH or led_status[1] == true) {
            MyServo.attach(servopin);
            MyServo.write(180);
            delay(500);
            MyServo.write(0);
            delay(500);
            MyServo.detach();
          }
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    h = altitude;
    //Messwerte an die serielle Schnittstelle zur Überprüfung übertragen, zum debuggen auskommentieren
    Serial.print(temperature);                       
    Serial.print(F("*C   "));
    Serial.print(pressure);    
    Serial.print(F("hPa   "));
    Serial.print("Höhe = ");
    Serial.print(altitude);
    Serial.println(" Meter");
    Serial.print("max. Flughöhe = ");
    Serial.print(hmax);
    Serial.println(" Meter");
    Serial.println(button1State);
    Serial.println(button2State);
    batterie = analogRead(A0); //analoge Spannung an A0 einlesen
    //Anzeige der maximal erreichten Flughöhe auf dem Display
      display.clearDisplay(); 
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print("max:  ");
      display.setTextSize(3);
     //ist die gemessene Höhe minus Starthöhe größer als die bisherige maximale Flughöhe --> neue maximale Flughöhe unter hmax speichern
      if ((round(h - h0)) > hmax)
          {  hmax = h - h0
          ;
          }
      
      display.print(String(hmax, 0));
      display.print(" m");
      display.display();
    Serial.print("H0 = ");
    Serial.print(h0);
    Serial.println(" Meter");
    Serial.print(" Diffenenz:  ");
    Serial.println(hmax - (h - h0));
    Serial.println (batterie);
    if ((hmax - (h - h0)) > hdiff)
          {   MyServo.attach(servopin);
            MyServo.write(180);
            delay(1000);
            MyServo.detach();
      
            MyServo.detach();
          
          }

          
    delay(50);//Pause zwischen den Messungen, 50 Millisekunden --> 20 Messungen pro Sekunde
      }
if (batterie > 380) //sobald der analoge Wert an A0 unter 30 sinkt die grüne LED aus und die rote LED einschalten
      {
        digitalWrite(ledgreen, HIGH);//LED ausschalten
        digitalWrite(ledred, LOW);//LED ausschalten
      }
else {
        digitalWrite(ledgreen, LOW);//LED ausschalten
        digitalWrite(ledred, HIGH);//LED ausschalten
}

   server.handleClient(); 
}

void handleRoot() {
  led0off();
}
void led0on(){
  led_status[0] = true;
  switchLEDAndSend(0,1);
}
void led0off(){
  led_status[0] = false;
  switchLEDAndSend(0,0);
}
void led1on(){
  led_status[1] = true;
  switchLEDAndSend(1,1);
}
void led1off(){
  led_status[1] = false;
  switchLEDAndSend(1,0);

                                                                                                        
}

void switchLEDAndSend(int num, bool state){
  String message = "";
  message += headAndTitle;
  message += "<div align=\"center\";>";
      digitalWrite(led[num], state);
  (led_status[0]==true)?(message += led0_0):(message += led0_1);

  

  message += "</BR><button class=\"button blueButton\">";
  message += "max. Flugh&oumlhe [m]: </BR>";
  message += String(hmax, 0);
  message += "</button>";
  message += "</BR><button class=\"button blueButton\">";
  message += "Flugh&oumlhe [m]: </BR>";
  message += String(h-h0, 0);
  message += "</button>";
  message += "</BR><button class=\"button blueButton\">";
  message += "Starth&oumlhe [m]: ";
  message += String(altitude, 0);
  message += "</BR>";
  message += "Temp [*C]:";
  message += String(temperature, 1);
  message += "</BR>";
  message += "Druck [hPA]: ";
  message += String(pressure, 2);
  message += "</BR>";
  message += "Batterie [%]: ";
  message += String((batterie-360)*1.1 , 0);
  message += "</button>";



  (led_status[1]==true)?(message += led1_0):(message += led1_1);

  server.send(200, "text/html", message); 
}
