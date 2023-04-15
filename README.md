# Hoehenmesser_Wasserrakete
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
