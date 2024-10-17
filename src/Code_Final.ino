//Definition des librairies
#include <ChainableLED.h>
#include <SPI.h>
#include <SD.h>
#include <Bonezegei_DS1307.h>
#define NUM_LEDS  5
#include <EnvironmentCalculations.h>                  
#include <BME280I2C.h>
#include <Wire.h>



// LEDS
ChainableLED leds(7, 8, NUM_LEDS);  

// BOUTONS
const int greenButtonPin = 3;
const int redButtonPin = 2;

// CAPTEURS
const int tmpSensorPin = A5;
String gpsData;
bool t;
Bonezegei_DS1307 rtc(0x68);

// Variables pour boucle capteurs
unsigned long previousMillis = 0; // will store last time LED was updated

// Variables pour boutons
volatile bool greenButtonPressed = false;
unsigned long greenButtonPressStartTime = 0;
volatile bool redButtonPressed = false;
unsigned long redButtonPressStartTime = 0;
const unsigned long longPressDuration = 2000; // on appuie 2s

// Globals divers
int mode = 1;
int previousMode = 0;
unsigned long loopSpeed = 60000; // Vitesse de la boucle de lecture des capteurs
const int chipSelect = 4;
unsigned long TempsInactif;

// Valeur par default capteur d'humiditer
float referencePressure = 1018.6;  
float outdoorTemp = 4.7;           
float barometerAltitude = 1650.3;  

// Biblithèque concernant le capteur d'humiditer
BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76
);
BME280I2C bme(settings);


void setup()
{
    Serial.begin(9600);
    ClearMoniteurSerie();

    pinMode(greenButtonPin, INPUT_PULLUP);                                //Initialisation des pin d'entrées
    pinMode(redButtonPin, INPUT_PULLUP);
    
    rtc.begin();
    rtc.setFormat(24);        //Set 12 Hours Format
    rtc.setAMPM(0);           //Set AM or PM    0 = AM  1 =PM
    rtc.setTime("10:45:00");  //Set Time    Hour:Minute:Seconds
    rtc.setDate("3/19/24");   //Set Date    Month/Date/Year

    // Interrupts
    attachInterrupt(digitalPinToInterrupt(greenButtonPin), greenButtonInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(redButtonPin), redButtonInterrupt, CHANGE);

    // Open serial communications and wait for port to open:
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.print("Initializing SD card...");
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
    }
    Serial.println("card initialized."); 
      
    if (digitalRead(redButtonPin) == LOW)                             // Permet l'initialisation de mode configuration 
    {
      mode = 3;
    }
    else
    {
      mode = 1;
    }
    SetHumidity();                                                   // Permet l'initialisation du capteur d'humidité
}

void setLedsToMode(int pmode)                                        // Définition des differents modes 
{
    switch (pmode)
    {
    case 0:
        // Maintenance - ORANGE
        leds.setColorRGB(0, 255, 69, 0);  
      
        LightSensor();
        Clock();
        printBME280Data();
        //gpsSensor();
        delay(1500);

        loopSpeed = 60000;
        break;

    case 1:
        // STD - GREEN
        leds.setColorRGB(0, 0, 255, 0);

        LightSensor();
        Clock();
        printBME280Data();
        //gpsSensor();
        MEMORY_CARD();
        delay(1500);

        loopSpeed = 1000;
        break;

    case 2:
        // ECO - BLUE
        leds.setColorRGB(0, 0, 0, 255); 

        LightSensor();
        Clock();
        printBME280Data();
        //gpsSensor();
        MEMORY_CARD();
        delay(3000);
        
        loopSpeed = 3000;
        break;

     case 3:

        // CONFIGURATION - YELLOW
        leds.setColorRGB(0, 255, 255, 0);  

        if ((millis() - TempsInactif) > (30 * 1000)) 
        {
        Serial.println(F("Fin de la configuration. Retour au mode standard."));
        mode = 1;
        return;
        }

        if (Serial.available() > 0)
        {
        String commande = Serial.readStringUntil('\n');            // \n pour arduino et \r pour VSCode PlatformIO
        int IndexEgal = commande.indexOf('=');
    
        if (commande == F("VERSION"))
        {
        Serial.println(F("Version du programme : 12.4"));
        Serial.println(F("Numéro de lot : 87"));
        }
        else if (commande == F("TIMEOUT"))
        {
        Serial.println(F("Erreur capteur"));
        }
        else if (commande == F("RESET"))
        {
        Serial.println(F("Réinitialisation de toutes les valeurs par default"));
        }
        else if (commande == F("Clock"))
        {
        Clock();
        }
        else if (commande == F("TEMPS"))
        {
        Serial.print("Temp: "); 
        Serial.println(outdoorTemp);
        Serial.print("Pressure: "); 
        Serial.println(referencePressure);
        Serial.print("Altitude: "); 
        Serial.println(barometerAltitude);
        }
        else if (commande == F("LUM"))
        {
        LightSensor();
        }
        else
        {
        Serial.println(F("Commande inexistante."));
        }
        return;
        }
        loopSpeed = 3000;
        break;
    }
}



void loop()                                                               //Verification des pressions des boutons
{

    if (redButtonPressed)     
    {
        unsigned long buttonPressDuration = millis() - redButtonPressStartTime;

        if (buttonPressDuration >= longPressDuration)
        {

            if (mode != 0)
            {
                // on a un mode, on le sauvegarde
                previousMode = mode;
                mode = 0;
            }
            else if (mode == 0 && previousMode == 0)
            { // pas de previous mode, on passe en mode STD
                mode = 1;
            }
            else if (mode == 0 && previousMode != 0)
            { // on a un previous mode, on le reprend
                mode = previousMode;
            }
          	setLedsToMode(mode);
          	redButtonPressed = false;
        }


    }
    else if (greenButtonPressed)                                         // Rouge prends la priorité sur vert
    {
        unsigned long buttonPressDuration = millis() - greenButtonPressStartTime;

        if (buttonPressDuration >= longPressDuration)
        {
            mode += 1;
            if (mode > 2)
                mode = 1;

            setLedsToMode(mode);
            greenButtonPressed = false;
        }
    }
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= loopSpeed)
    {
        previousMillis = currentMillis;
    }
    setLedsToMode(mode);
}

const unsigned long debounceDelay = 50;
volatile unsigned long greenLastInterruptTime = 0;


void greenButtonInterrupt()                                     // Fonction interuption appeller dans le setup pour le bouton vert
{
    unsigned long interruptTime = millis();

    if (interruptTime - greenLastInterruptTime > debounceDelay)
    {

        if (digitalRead(greenButtonPin) == LOW)
        {
            greenButtonPressed = true;
            greenButtonPressStartTime = millis();
        }
        else
        {
            greenButtonPressed = false;
            greenButtonPressStartTime = 0;
        }
        greenLastInterruptTime = interruptTime;
    }
}

volatile unsigned long redLastInterruptTime = 0;

void redButtonInterrupt()                                       // Fonction interuption appeller dans le setup pour le bouton rouge 
{
    unsigned long interruptTime = millis();

    if (interruptTime - redLastInterruptTime > debounceDelay)
    {

        if (digitalRead(redButtonPin) == LOW)
        {
            redButtonPressed = true;
            redButtonPressStartTime = millis();
        }
        else
        {
            redButtonPressed = false;
            redButtonPressStartTime = 0;
        }
        redLastInterruptTime = interruptTime;
    }
}


void LightSensor(){                                              // Lecture de capteur de luminosité
  int sensor = analogRead(A0);
  Serial.print("Brightness : ");
  Serial.println(sensor);
}

void MEMORY_CARD(){                                              // Enregistrement sur la carte SD

   float temp(NAN), hum(NAN), pres(NAN);
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);
   bme.read(pres, temp, hum, tempUnit, presUnit);
   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;
   float seaLevel = EnvironmentCalculations::EquivalentSeaLevelPressure(barometerAltitude, temp, pres, envAltUnit, envTempUnit);
   float absHum = EnvironmentCalculations::AbsoluteHumidity(temp, hum, envTempUnit);
   float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, outdoorTemp, envTempUnit);
   float heatIndex = EnvironmentCalculations::HeatIndex(temp, hum, envTempUnit);
   float dewPoint = EnvironmentCalculations::DewPoint(temp, hum, envTempUnit);

   int sensor = analogRead(A0);
   File dataFile = SD.open("datalog.txt", FILE_WRITE);     

   if (dataFile) { 
    dataFile.print("Brightness: "); 
    dataFile.print(sensor);
    dataFile.print("  ");
    dataFile.print("Heure : ");
    dataFile.print(rtc.getHour());
    dataFile.print(":");
    dataFile.print(rtc.getMinute());
    dataFile.print(":");
    dataFile.print(rtc.getSeconds());
    dataFile.print("  ");
    dataFile.print("Date : ");
    dataFile.print(rtc.getDate());
    dataFile.print("/");
    dataFile.print(rtc.getMonth());
    dataFile.print("/");
    dataFile.print(rtc.getYear());
    dataFile.print("  ");

    dataFile.print("Temp: ");
    dataFile.print(temp);
    dataFile.print("  ");
    dataFile.print("Humidity: ");
    dataFile.print(hum);
    dataFile.print("  ");
    dataFile.print("Pressure: ");
    dataFile.print(pres);
    dataFile.print("  ");
    dataFile.print("Altitude: ");
    dataFile.println(altitude);
    dataFile.print("  ");
    //dataFile.print("Dew point: ");
    //dataFile.print(dewPoint);
    //dataFile.print("Sea Level Pressure: ");
    //dataFile.print(seaLevel);
    //dataFile.print("Heat Index: ");
    //dataFile.print(heatIndex);

    dataFile.close();
    }

    else {
    Serial.println("error opening datalog.txt");           
    }

}


/*void gpsSensor(){                                               // Récuparation des données GPS
    if (SoftSerial.available())
    {
        t=true;
      while(t){
        gpsData = SoftSerial.readStringUntil('\n');
        if (gpsData.startsWith("$GNGLL",0)){
          t=false;
        }
      }
    }
    Serial.println(gpsData);
}*/

void Clock(){                                                    // Recupération des données de l'horloge

  if (rtc.getTime()) {
    Serial.print("Heure : ");
    Serial.print(rtc.getHour());
    Serial.print(":");
    Serial.print(rtc.getMinute());
    Serial.print(":");
    Serial.println(rtc.getSeconds());

    Serial.print("Date : ");
    Serial.print(rtc.getDate());
    Serial.print("/");
    Serial.print(rtc.getMonth());
    Serial.print("/");
    Serial.println(rtc.getYear());
  }
  delay(500);
}

void ClearMoniteurSerie(){                                      // Permet de nettoyer le port serie
  for (int i = 0; i < 5; i++)
  {
    Serial.println();
  }
}


void SetHumidity(){                                             //Permet d'initialiser le capteur de l'humidité

  Serial.begin(9600);

  while(!Serial) {} // Wait

  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
}



void printBME280Data() {                                   // Permet de recuperer les données du capteur d'humiditer 

   float temp(NAN), hum(NAN), pres(NAN);

   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);

   bme.read(pres, temp, hum, tempUnit, presUnit);

   Serial.print("Temp: ");
   Serial.print(temp);
   Serial.print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? "C" :"F"));
   Serial.print("\t\tHumidity: ");
   Serial.print(hum);
   Serial.print("% RH");
   Serial.print("\t\tPressure: ");
   Serial.print(pres);
   Serial.print(String(presUnit == BME280::PresUnit_hPa ? "hPa" : "Pa")); // expected hPa and Pa only

   EnvironmentCalculations::AltitudeUnit envAltUnit  =  EnvironmentCalculations::AltitudeUnit_Meters;
   EnvironmentCalculations::TempUnit     envTempUnit =  EnvironmentCalculations::TempUnit_Celsius;

   /// To get correct local altitude/height (QNE) the reference Pressure
   ///    should be taken from meteorologic messages (QNH or QFF)
   float altitude = EnvironmentCalculations::Altitude(pres, envAltUnit, referencePressure, outdoorTemp, envTempUnit);

   float dewPoint = EnvironmentCalculations::DewPoint(temp, hum, envTempUnit);

   /// To get correct seaLevel pressure (QNH, QFF)
   ///    the altitude value should be independent on measured pressure.
   /// It is necessary to use fixed altitude point e.g. the altitude of barometer read in a map
   float seaLevel = EnvironmentCalculations::EquivalentSeaLevelPressure(barometerAltitude, temp, pres, envAltUnit, envTempUnit);

   float absHum = EnvironmentCalculations::AbsoluteHumidity(temp, hum, envTempUnit);

   Serial.print("\t\tAltitude: ");
   Serial.print(altitude);
   Serial.print((envAltUnit == EnvironmentCalculations::AltitudeUnit_Meters ? "m" : "ft"));
   Serial.print("\t\tDew point: ");
   Serial.print(dewPoint);
   Serial.print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   Serial.print("\t\tEquivalent Sea Level Pressure: ");
   Serial.print(seaLevel);
   Serial.print(String( presUnit == BME280::PresUnit_hPa ? "hPa" :"Pa")); // expected hPa and Pa only
   Serial.print("\t\tHeat Index: ");
   float heatIndex = EnvironmentCalculations::HeatIndex(temp, hum, envTempUnit);
   Serial.print(heatIndex);
   Serial.print("°"+ String(envTempUnit == EnvironmentCalculations::TempUnit_Celsius ? "C" :"F"));
   Serial.print("\t\tAbsolute Humidity: ");
   Serial.println(absHum);

   delay(1000);
}

