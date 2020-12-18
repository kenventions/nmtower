
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS.h>

// *************** USER VARIABLES ***************

// USER WiFi Parameters
// --------------------
const char* ssid = "yourWiFiSSID";
const char* password = "yourWiFiPSWD";

// USER ADSBx "nmTower" Parameters
// -------------------------------
const char* adsbxkey = "putyourADSBxkeyhere";
const int adsbxrange = 10; // search distance in nm (integer)
const String adsbxlat = "32.897299"; // search origin LAT, using String for better precision in ADSBx URL (vs float)
const String adsbxlon = "-97.040453"; // search origin LON, using String for better precision in ADSBx URL (vs float)

// USER Taxi Filter Parameters
// ---------------------------
const int minspeed = 30; // taxiing planes below this speed will not be displayed (integer)

// USER Display Dimming Parameters
// -------------------------------
const bool dimzero = true; // (true or false) dims the display when no aircraft are detected (crude night dimmer)

// **********************************************

// define URL to get ADSBX data
String adsbxurl = "https://adsbexchange.com/api/aircraft/json/lat/" + adsbxlat + "/lon/" + adsbxlon + "/dist/" + String(adsbxrange);

// initialize LCD display
#include <SerLCD.h> //Click here to get the library: http://librarymanager/All#SparkFun_SerLCD
SerLCD lcd; // Initialize the library with default I2C address 0x72
bool dimmedstate = false;

// initialize JSON settings
const size_t capacity = 60000;
DynamicJsonDocument doc(capacity);

// function for Near My Tower (nmTower) graphic
void tower() {
  lcd.clear(); 
  lcd.print("nmTOWER   o   o");
  lcd.setCursor(0, 1);
  lcd.print("=======  <T> /T>");
  delay(3000);
}

// function for RADAR animation
void radar() {
  lcd.setCursor(0, 0);
  lcd.print("]           -<-'");
  lcd.setCursor(0, 1);
  lcd.print("I               ");
  delay(1000);
  lcd.setCursor(1, 0);lcd.print(")");
  for (int i = 0; i <= 9; i++) {
    lcd.setCursor(2+i, 0);lcd.print(")");
    lcd.setCursor(1+i, 0);lcd.print(" ");
  }
  for (int i = 0; i <= 9; i++) {
    lcd.setCursor(10-i, 0);lcd.print("(");
    lcd.setCursor(11-i, 0);lcd.print(" ");
  }
  lcd.setCursor(0, 0);
  lcd.print("]           -<-'");
  delay(250);
}  

// program intro (executed once)
void setup() {
  // LCD prep 
  Wire.begin();
  lcd.begin(Wire); //Set up the LCD for I2C communication
  lcd.setBacklight(200, 170, 255); //Set backlight to bright white
  lcd.setContrast(10); //Set contrast. Lower to 0 for higher contrast.
  delay(1000);

  tower();

  lcd.clear(); 
  lcd.print("LAT: ");lcd.print(adsbxlat);
  lcd.setCursor(0, 1);
  lcd.print("LON: "); lcd.print(adsbxlon);
  delay(3000);

  // start serial comms & WiFi
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("WiFi search ...");
    delay(3000);
    lcd.clear();
    lcd.print("ID=" + String(ssid));
    lcd.setCursor(0, 1);
    lcd.print("PW=" + String(password));
    delay(3000);
  }
  lcd.clear();
Serial.println("setup complete");
}

// main program loop
void loop() {

    tower();

  // Check WiFi Status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;  //Object of class HTTPClient

    // Submit adsbx api request
    http.begin(adsbxurl);
    http.addHeader("api-auth", adsbxkey);
    int httpCode = http.GET();
    //Check the returning code                                                                  
    if (httpCode > 0) {
      // Get the request response payload
      String payload = http.getString();
      http.end();   //Close connection
      
      // Serial.println(payload);
      
      deserializeJson(doc, payload);
      JsonArray currList = doc["ac"]; // Get a reference to the array

      Serial.println("json complete");

      //determine dimming profile
      if(dimzero==true) {
        if(currList.size()==0 && dimmedstate==false) {
          lcd.setBacklight(10, 9, 20); //Set backlight to dim blue
          lcd.clear();
          dimmedstate=true;
        } else {
            if(currList.size()!=0 && dimmedstate==true) {
              lcd.setBacklight(200, 170, 255); //Set backlight to bright white
              lcd.clear();
              dimmedstate=false;
            }
          }
      }

      // display plane count and search distance
      radar();
      lcd.setCursor(0, 1);
      lcd.print("Planes: " + String(currList.size()) + " <" +String(adsbxrange) +"nm");
      delay(3000);

      if(currList.size()!=0) {

        // Print plane details in serial monitor for debugging
        // for (JsonObject currPlane : currList) {
          //Serial.print(" call- "); Serial.print(currPlane["call"].as<String>());
          //Serial.print(", type: "); Serial.print(currPlane["type"].as<String>());
          //Serial.print(", alt: "); Serial.print(currPlane["alt"].as<int>());
          //Serial.print(", spd: "); Serial.print(currPlane["spd"].as<int>());
          //Serial.print(", trak: "); Serial.print(currPlane["trak"].as<int>());
          //Serial.print(", dst: "); Serial.println(currPlane["dst"].as<float>());
          //  Serial.print(", from: "); Serial.print(currPlane["from"].as<String>());
          //  Serial.print(", to: "); Serial.print(currPlane["to"].as<String>());
        // }
        
        //Serial.println(adsbxurl);
        //Serial.println(currList.size());

        // find nearest plane
        int np = 0;
        float tempdst = 0;
    
        if (currList.size()>1) {
          for (int i = 0; i < currList.size()-1; i++) {
            if (currList[np]["spd"].as<float>()>float(minspeed)) {
              if ((currList[np]["dst"].as<float>() > currList[i+1]["dst"].as<float>()) 
                && currList[i+1]["spd"].as<float>()>float(minspeed)) {
                np = i+1;
              }
            } else {np++; i++;};   
            Serial.print(np); Serial.print(" "); Serial.print(currList[np]["call"].as<String>());
            Serial.print(" "); Serial.print(currList[np]["dst"].as<float>());
            Serial.print(" "); Serial.println(currList[np]["spd"].as<float>());
          }
        } 

        // determine bearing to plane
        String npBear="N";
        float targetBear = TinyGPS::course_to(adsbxlat.toFloat(),adsbxlon.toFloat(),currList[np]["lat"],currList[np]["lon"]);
        if (targetBear>337.5 || targetBear<=22.5) {npBear="N";}
        if (targetBear>22.5 && targetBear<=67.5) {npBear="NE";}
        if (targetBear>67.5 && targetBear<=112.5) {npBear="E";}
        if (targetBear>112.5 && targetBear<=157.5) {npBear="SE";}
        if (targetBear>157.5 && targetBear<=202.5) {npBear="S";}
        if (targetBear>202.5 && targetBear<=247.5) {npBear="SW";}
        if (targetBear>247.5 && targetBear<=292.5) {npBear="W";}
        if (targetBear>292.5 && targetBear<=337.5) {npBear="NW";}
 
        // display plane details, page 1
        if (currList[np]["call"]=="") {currList[np]["call"]=currList[np]["icao"];}
        if (currList[np]["type"]=="") {currList[np]["type"]="no type";}
        lcd.clear();
        lcd.print(currList[np]["call"].as<String>());lcd.print(" / ");lcd.print(currList[np]["type"].as<String>());
        lcd.setCursor(0,1);
        lcd.print(currList[np]["dst"].as<float>(),1);lcd.print("nm ");lcd.print(npBear);lcd.print(" ");
        lcd.print(currList[np]["alt"].as<int>());lcd.print("'");
        Serial.println("page1 complete");
        delay(7000);

        // determine heading of plane
        String npHdg="N";
        if (currList[np]["trak"].as<float>()>337.5 || currList[np]["trak"].as<float>()<=22.5) {npHdg="N";}
        if (currList[np]["trak"].as<float>()>22.5 && currList[np]["trak"].as<float>()<=67.5) {npHdg="NE";}
        if (currList[np]["trak"].as<float>()>67.5 && currList[np]["trak"].as<float>()<=112.5) {npHdg="E";}
        if (currList[np]["trak"].as<float>()>112.5 && currList[np]["trak"].as<float>()<=157.5) {npHdg="SE";}
        if (currList[np]["trak"].as<float>()>157.5 && currList[np]["trak"].as<float>()<=202.5) {npHdg="S";}
        if (currList[np]["trak"].as<float>()>202.5 && currList[np]["trak"].as<float>()<=247.5) {npHdg="SW";}
        if (currList[np]["trak"].as<float>()>247.5 && currList[np]["trak"].as<float>()<=292.5) {npHdg="W";}
        if (currList[np]["trak"].as<float>()>292.5 && currList[np]["trak"].as<float>()<=337.5) {npHdg="NW";}

        // display plane details, page 2
        lcd.clear();
        lcd.print(currList[np]["call"].as<String>());
        lcd.setCursor(8,0);lcd.print("HDG= ");lcd.print(npHdg);
        lcd.setCursor(0,1); lcd.print("As ");lcd.print(currList[np]["spd"].as<float>(),0);
        lcd.setCursor(8,1);lcd.print("Vs ");lcd.print(currList[np]["vsi"].as<int>());
        Serial.println("page2 complete");
        delay(7000);

        // display plane details, page 3
        String depID = currList[np]["from"].as<String>();depID.remove(3);
        String arvID = currList[np]["to"].as<String>();arvID.remove(3);
        if (depID == "nul") {depID=" -"; arvID=" -";}
        lcd.clear();
        lcd.print(currList[np]["call"].as<String>());lcd.setCursor(8,0);lcd.print("DEP->ARV");
        lcd.setCursor(0,1);lcd.print(currList[np]["type"].as<String>());
        lcd.setCursor(8,1);lcd.print(depID);lcd.setCursor(13,1);lcd.print(arvID);
        Serial.println("page3 complete");
        delay(7000);

      } else { // no planes in search area
        Serial.println("no planes");
        delay(30000);  
      }
            
    } else { // no ADSBX response

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("No ADSBx reply!");
      Serial.println("no ADSBx reply");
      delay(2000);
    }
    
  } else { // no WIFI connection

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("WIFI lost ...");
    Serial.println("no WiFi");
    delay(2000);  
  }
Serial.println("loop complete");
Serial.println("***"); 
 
} // end of main program loop
