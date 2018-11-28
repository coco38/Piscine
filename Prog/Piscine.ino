#include <ZUNO_OneWire.h>
#include <ZUNO_LCD_I2C.h>
#include <ZUNO_DS18B20.h>

/*******************************************
 * AFFECTATION DES PINS
 ******************************************/
//TEST
#define PIN_LED 13
#define PIN_BUTTON 18

//Entrée Ana
#define PIN_ORP         A0    // Pin de la sonde ORP
#define PIN_PH          A1    // Pin de la sonde PH
#define PIN_ORP_ETAL    A2    // Pin pour l'étalonnage de la sonde ORP
#define PIN_PH_ETAL     A3    // Pin pour l'étalonnage de la sonde PH

//Température
#define PIN_TEMP_LOCAL  11 // Pin du capteur DS18B20 : Température du local 
#define PIN_TEMP_EAU    12 // Pin du capteur DS18B20 : Température de l'eau

//Entrée TOR
#define PIN_MODE_MANU   14    // Pin du mode Manu
#define PIN_MODE_AUTO   15    // Pin du mode Auto
#define PIN_HORAIRE     16    // Pin du selecteur horaire
#define PIN_LUMIERE     17    // Pin du bouton lumiere

//Sortie relais
#define PIN_ECL_PISCINE   19  // Pin de l'éclairage de la piscine
#define PIN_POMP_PISCINE  20  // Pin de la pompe de la piscine
#define PIN_ELECTROLYSE   21  // Pin de l'electrolyseur

/*******************************************
 * CONSTANTES
 ******************************************/
//tension de reference (Vcc) du Zuno
#define VOLT_REFERENCE  3.05  // Tension de référence
 
//température
#define TEMP_GEL_MIN  -0.5  // Température de démarrage du mode antigel
#define TEMP_GEL_MAX   1.0  // Température d'arrêt du mode antigel
 
//ORP/Redox
#define ORP_MIN     660   // Tension (mV) de démarrage de la production de chlore
#define ORP_MAX     720   // Tension (mV) d'arrêt de la production de chlore

#define TempoMesure 300000  //attente entre chaque mesure
/******************************************
 * VARIABLES GLOBALES
 ******************************************/
//TEST
byte LedState;

//temporisation
unsigned long PrecMillis = 0;

//ecran lcd
LiquidCrystal_I2C lcd(0x3F, 20, 4);

//mode de fonctionnement
byte modePompe = 0; // 0:arret / 1:marche forcée / 2:auto
byte modePompePrec = 0;

//mesure de temperature
//Local
OneWire owTempLoc(PIN_TEMP_LOCAL);
DS18B20Sensor CapteurTempLoc(&owTempLoc);
int TemperatureLoc=0;
//Eau
OneWire owTempEau(PIN_TEMP_EAU);

DS18B20Sensor CapteurTempEau(&owTempEau);
int TemperatureEau=0;

//mesures
int valORP=0;
float valPH=0.0;

//sorties relais
//pompe
bool fctGel = false;
bool pinHoraire = false;
bool pinHorairePrec = false;
bool etatPompe = false;

//lumiere
bool pinLumiere = false;
bool pinLumierePrec = false;
bool etatLumiere = false;

//Chlore
bool etatChloreProd = false;

/******************************************
 * CANAUX ZWAVE
 * 1 : cmde de la pompe
 * 2 : cmde de la lumiere
 * 3 : mode AUTO (2) / MANU (1) / ARRET (0)
 * 4 : etat de la demande horaire
 * 5 : etat de la demande hors gel
 * 6 : température du local
 * 7 : température de l'eau
 * 8 : niveau ORP/Redox
 * 9 : niveau PH
 * 10 : etat production de Chlore
 ******************************************/

ZUNO_SETUP_CHANNELS(
  //Etat & commande de la pompe
  ZUNO_SWITCH_BINARY(getPompe, setPompe),
  //Etat & commande de la lumiere  
  ZUNO_SWITCH_BINARY(getLumiere, setLumiere),
  //Mode selectionne
  ZUNO_SENSOR_MULTILEVEL_GENERAL_PURPOSE(getMode),
  //Etat & commande de la pompe
  ZUNO_SENSOR_BINARY(ZUNO_SENSOR_BINARY_TYPE_GENERAL_PURPOSE, getHoraire),
  //Etat du mode hors-gel
  ZUNO_SENSOR_BINARY(ZUNO_SENSOR_BINARY_TYPE_GENERAL_PURPOSE, getGel),
  //Temperature du local
  ZUNO_SENSOR_MULTILEVEL(ZUNO_SENSOR_MULTILEVEL_TYPE_TEMPERATURE,SENSOR_MULTILEVEL_SCALE_CELSIUS, SENSOR_MULTILEVEL_SIZE_TWO_BYTES, SENSOR_MULTILEVEL_PRECISION_ONE_DECIMAL, getTempLocal),
  //Temperature de l'eau
  ZUNO_SENSOR_MULTILEVEL(ZUNO_SENSOR_MULTILEVEL_TYPE_TEMPERATURE,SENSOR_MULTILEVEL_SCALE_CELSIUS, SENSOR_MULTILEVEL_SIZE_TWO_BYTES, SENSOR_MULTILEVEL_PRECISION_ONE_DECIMAL, getTempEau),
  //ORP/Redox
  ZUNO_SENSOR_MULTILEVEL(ZUNO_SENSOR_MULTILEVEL_TYPE_VOLTAGE,SENSOR_MULTILEVEL_SCALE_VOLT, SENSOR_MULTILEVEL_SIZE_TWO_BYTES, SENSOR_MULTILEVEL_PRECISION_ZERO_DECIMALS, getORP),
  //Ph
  ZUNO_SENSOR_MULTILEVEL(ZUNO_SENSOR_MULTILEVEL_TYPE_VOLTAGE,SENSOR_MULTILEVEL_SCALE_VOLT, SENSOR_MULTILEVEL_SIZE_TWO_BYTES, SENSOR_MULTILEVEL_PRECISION_ONE_DECIMAL, getPH),
  //Etat de la production de chlore
  ZUNO_SENSOR_BINARY_GENERAL_PURPOSE(getChloreProd));

/******************************************
 * INITIALISATION
 ******************************************/
void setup() {
  //initialisation port serie
  Serial.begin(9600);
  Serial.println("Start...");

  Serial.println("Initialisation des sorties relais");
  pinMode(PIN_ECL_PISCINE, OUTPUT);
  pinMode(PIN_POMP_PISCINE, OUTPUT);
  pinMode(PIN_ELECTROLYSE, OUTPUT);

  Serial.println("Initialisation des entrees");
  pinMode(PIN_MODE_MANU, INPUT_PULLUP);
  pinMode(PIN_MODE_AUTO, INPUT_PULLUP);
  pinMode(PIN_HORAIRE, INPUT_PULLUP);
  pinMode(PIN_LUMIERE, INPUT_PULLUP);
        
  //initialisation écran lcd
  Serial.println("Initialisation de l'ecran LCD");
  delay(300);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("T");lcd.print((char)223);lcd.print(" Local:");
  lcd.setCursor(13,0);
  lcd.print("Eau:");
  lcd.setCursor(0,1);
  lcd.print("Redox:       ");
  lcd.setCursor(13,1);
  lcd.print("Ph:    ");
  lcd.setCursor(0,2);
  lcd.print("Mode:ARRET");
  lcd.setCursor(13,2);
  lcd.print("Lum:OFF");
  lcd.setCursor(0,3);
  lcd.print("Pompe:OFF Chlore:OFF"); 
  delay(100);
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  digitalWrite(PIN_LED, LOW);
  LedState=0;

  //initialisation des relais
  Serial.println("Initialisation des sorties relais");
  digitalWrite(PIN_ECL_PISCINE, HIGH);
  digitalWrite(PIN_POMP_PISCINE, HIGH);
  digitalWrite(PIN_ELECTROLYSE, HIGH);  

  PrecMillis = millis();
}

void loop() {

  lectureEntrees();
  
  if ((digitalRead(PIN_BUTTON)==0) || ((millis() - PrecMillis) >= TempoMesure)) {

    PrecMillis = millis();
    
    Serial.println("Button pressed");
    if(LedState==0){
      digitalWrite(PIN_LED, HIGH);
      LedState = 1;
      Serial.println("Led On");
    }
    else{
      digitalWrite(PIN_LED, LOW);
      LedState = 0;
      Serial.println("Led Off");    
    }

    //mesure de la température du local
    lectureTempLocal();    

    //si la pompe fonctionne, on fait les mesures de l'eau (ORP, PH, Temperature)
    if(etatPompe){

      lectureTempEau();
      lectureORP();
      lecturePH();
    }

    zunoSendReport(1);
    zunoSendReport(2);
    zunoSendReport(3);
    zunoSendReport(4);
    zunoSendReport(5);
    zunoSendReport(10);
  }

  //Mise à jour du fonctionnement de la pompe
  FctPompe();
  
  //pilotage du relais de la pompe et mise à jour de l'affichage LCD
  //Edit 29-Sep-2018
  if(etatPompe){
    digitalWrite(PIN_POMP_PISCINE,LOW);
    lcd.setCursor (6, 3);
    lcd.print("ON ");    
  }
  else{
    digitalWrite(PIN_POMP_PISCINE,HIGH);
    lcd.setCursor (6, 3);
    lcd.print("OFF");  
  }

  FctLumiere();
  
  //pilotage du relais de la lumiere et mise à jour de l'affichage LCD
  //Edit 29-Sep-2018 
  if(etatLumiere){
    digitalWrite(PIN_ECL_PISCINE, LOW);
    lcd.setCursor(17,2);
    lcd.print("ON ");
  }
  else {
    digitalWrite(PIN_ECL_PISCINE, HIGH);
    lcd.setCursor(17,2);
    lcd.print("OFF");    
  }

  FctProdChlore();
  
  //pilotage du relais de la production de chlore et mise à jour de l'affichage LCD
  //Edit 29-Sep-2018
  if(etatChloreProd){
    digitalWrite(PIN_ELECTROLYSE, LOW);
    lcd.setCursor (17, 3);
    lcd.print("ON ");
  }
  else {
    digitalWrite(PIN_ELECTROLYSE, HIGH);
    lcd.setCursor (17, 3);
    lcd.print("OFF");  
  }
  delay(120);
}

/****************************************************************************
 * FONCTION : récupère la température de l'eau du capteur DS18B20
 *  sans objet
 ****************************************************************************/
void lectureTempEau(){
  byte adresse[8];
  float temperature;
      
  //si le capteur est trouvé
  if(CapteurTempEau.scanAloneSensor(adresse)) {
      
      Serial.print("Adresse du capteur de temperature de l'eau ");
      for(int j = 0; j < 8; j++) {
        Serial.print(adresse[j], HEX);
        Serial.print(" ");
      }  
      Serial.println();  

      lcd.setCursor (17, 0);
      lcd.print("   "); // Clean lcd old digits
        
      //si la lecture est correcte
      temperature = CapteurTempEau.getTemperature(adresse);

      if((int)CapteurTempEau.getTemperature(adresse) != BAD_TEMP) {
        Serial.print("Temperature: ");
        Serial.println(temperature); 

        TemperatureEau = temperature;
        zunoSendReport(7);

        lcd.setCursor (17, 0);
        lcd.print((int)temperature);
        lcd.print((char)223); 
      }
      else {
        Serial.println("Temperature de l'eau erronee");
        
        lcd.setCursor (17, 0);
        lcd.print("TMP");
      }
    }
    else {
      Serial.println("Capteur de temperature de l'eau non trouve!");
      
      lcd.setCursor (17, 0);
      lcd.print("CAP");
    }

}

/****************************************************************************
 * FONCTION : récupère la température du local du capteur DS18B20
 *  sans objet
 ****************************************************************************/
void lectureTempLocal(){
  byte adresse[8];
  float temperature;
      
  //si le capteur est trouvé
  if(CapteurTempLoc.scanAloneSensor(adresse)) {
      
      Serial.print("Adresse du capteur de temperature du local ");
      for(int j = 0; j < 8; j++) {
        Serial.print(adresse[j], HEX);
        Serial.print(" ");
      }  
      Serial.println();  

      lcd.setCursor (9, 0);
      lcd.print("    "); // Clean lcd old digits
        
      //si la lecture est correcte
      temperature = CapteurTempLoc.getTemperature(adresse);

      if((int)CapteurTempLoc.getTemperature(adresse) != BAD_TEMP) {
        Serial.print("Temperature: ");
        Serial.println(temperature); 

        TemperatureLoc = temperature;
        zunoSendReport(6);
    
        lcd.setCursor (9, 0);
        lcd.print((int)temperature);
        lcd.print((char)223); 
      }
      else {
        Serial.println("Temperature du local erronee");
  
        lcd.setCursor (9, 0);
        lcd.print("TMP");
      }
    }
    else {
      Serial.println("Capteur de temperature du local non trouve!");

      lcd.setCursor (9, 0);
      lcd.print("CAP");
    }

}

/****************************************************************************
 * FONCTION : active la pompe de la piscine en cas de gel
 *  sans objet
 ****************************************************************************/
void FctPompe(){
  bool etatPompePrec = etatPompe;
  bool fctGelPrec = fctGel;
  
  //si la température du local < TEMP MIN => Gel
  if(TemperatureLoc < TEMP_GEL_MIN){
    fctGel = true;
  }
  if(TemperatureLoc > TEMP_GEL_MAX){
    fctGel = false;
  }

  //selon le MODE selectionné
  switch(modePompe){
    //ARRET
    case 0:
      etatPompe = false;
      break;
    //MARCHE FORCEE
    case 1:
      etatPompe = true;
      break;
    //AUTO
    case 2:
      //si le mode Auto vient d'être activé => on prend l'état de la demande horaire
      if(modePompe && !modePompePrec){
        etatPompe = pinHoraire;
      }

      //si le mode hors gel est activé => marche de la pompe en priorité
      if(fctGel){
        etatPompe = true;
      }
      else{
        //demande d'arrêt de la pompe SI
        //Arret_Horaire OU (Fin du mode Hors Gel ET pas de Demande_Horaire
        if((!pinHoraire && pinHorairePrec) || (!fctGel && fctGelPrec && !pinHoraire)){
          etatPompe = false;          
        }
        //demande de démarrage de la pompe SI
        //Arret_Horaire OU (Fin du mode Hors Gel ET pas de Demande_Horaire        
        if(pinHoraire && !pinHorairePrec) {
          etatPompe = true;
        }
      }
      break;
    default:
      etatPompe = false;
  }

  //si l'état de la pompe à changer, on signale au controleur zwave
  if(etatPompe != etatPompePrec){
    zunoSendReport(1);
    //Edit 29-Sep-2018
    //Serial.print("Passage de la pompe : "); Serial.println(etatPompe);
    //lcd.setCursor (6, 3);
    //if(etatPompe ==  false){
    //  lcd.print("OFF");
    //}
    //else {
    //  lcd.print("ON ");      
    //}
  }

  //si le mode à changer, on signale au controleur zwave et on met à jour l'affichage LCD
  if(modePompe != modePompePrec){
    zunoSendReport(3);
    Serial.print("Passage en mode : "); Serial.println(modePompe);

    lcd.setCursor (5, 2);
    switch(modePompe){
      case 0:
        lcd.print("ARRET ");
        break;
      case 1:
        lcd.print("MARCHE");
        break;
      case 2:
        lcd.print("AUTO  ");
        break;
    }
  }  

  //si le mode hors gel à changer, on signale au controleur zwave
  if(fctGel != fctGelPrec){
    zunoSendReport(5);
    Serial.print("hors gel : "); Serial.println(fctGel);    
  }
  
  //Memorise l'état des entrées
  pinHorairePrec = pinHoraire;
  modePompePrec = modePompe;
}

/****************************************************************************
 * FONCTION : change l'état de la lumiere à chaque appui sur le bouton
 *  sans objet
 ****************************************************************************/
void FctLumiere(){
  if(pinLumiere && !pinLumierePrec){
    etatLumiere = !etatLumiere;
    Serial.print("Appui sur le bouton lumiere: ");Serial.println(etatLumiere);
    zunoSendReport(1);
  }

  pinLumierePrec = pinLumiere;
}

/****************************************************************************
 * FONCTION : regule le chlore de la piscine
 *  sans objet
 ****************************************************************************/
void FctProdChlore(){
    bool etatChloreProdPrec;

    etatChloreProdPrec = etatChloreProd;
    // si le niveau de chlore est trop bas, 
    // ET la pompe fonctionne 
    // ET la température de l'eau est supérieur à 15°C
    // => on demarre la production de chlore
    if((valORP < ORP_MIN) && (etatPompe) && (TemperatureEau > 15.0)){
      etatChloreProd = true;
    }
    // si le niveau de chlore est trop haut
    // OU la pompe ne fonctionne pas 
    // OU la température de l'eau est inférieur à 15°C
    // => on arrete la production de chlore
    if((valORP > ORP_MAX)||(!etatPompe) || ((TemperatureEau < 15.0))){
      etatChloreProd = false;      
    }

    //si le etat de la production de chlore a changé, on signale au controleur zwave et on met à jour l'affichage LCD
    if(etatChloreProd != etatChloreProdPrec){
      zunoSendReport(10);
      //Edit 29-Sep-2018
      //Serial.print("Electrolyseur: "); Serial.println(etatChloreProd);    
      //lcd.setCursor (17, 3);
      //if(etatChloreProd ==  false){
      //  lcd.print("OFF");
      //}
      //else {
      //  lcd.print("ON ");      
      //}
    }
  
}

/****************************************************************************
 * FONCTION : lit les entrées et les stock dans des variables
 *  sans objet
 ****************************************************************************/
void lectureEntrees(){
  bool modeAuto;
  bool modeManu;
  
  pinHoraire = !digitalRead(PIN_HORAIRE);
  modeManu = !digitalRead(PIN_MODE_MANU);
  modeAuto = !digitalRead(PIN_MODE_AUTO);
  modePompe = (byte)((modeAuto <<1)|(modeManu));
  pinLumiere = !digitalRead(PIN_LUMIERE);

  //si la demande horaire a changé
  if(pinHoraire != pinHorairePrec){
      zunoSendReport(4); 
  }

  //si l'état de la lumiere a changé
  if(pinLumiere != pinLumierePrec){
      zunoSendReport(2);
  }

}

/****************************************************************************
 * FONCTION : lit la sonde ORP/Redox
 *  sans objet
 ****************************************************************************/
void lectureORP(){
    int valueADC;
    float value,ValueEtalon;
        
    valueADC = analogRead(PIN_ORP);
    value = (float)(valueADC) * 1.55 * 1000 * VOLT_REFERENCE / 1024; //1,55: rapport du pont diviseur
    value = (2080-value)/0.875;
    valueADC = analogRead(PIN_ORP_ETAL);
    ValueEtalon = ((float)(valueADC) * 600 / 1024) - 400;
    valORP = value + ValueEtalon;

    Serial.print("ORP: ");Serial.println(valORP);

    //Edit 29-Sep-2018
    lcd.setCursor(0,1);
    lcd.print("Redox:       ");
    lcd.setCursor (6, 1);
    lcd.print(valORP);
    lcd.print("mV");
    zunoSendReport(8);

}

/****************************************************************************
 * FONCTION : lit lit la sonde PH
 *  sans objet
 ****************************************************************************/
void lecturePH(){
    int valueADC;
    float value,ValueEtalon;
    
    valueADC = analogRead(PIN_PH)*VOLT_REFERENCE;
    value = (float)(valueADC) / 1024 * 1.55; //1,55: rapport du pont diviseur
    valueADC = analogRead(PIN_PH_ETAL);
    ValueEtalon = ((float)(valueADC) / 1024 * 4)-1;
    valPH = 7-((2.5-value)/(0.257179+0.000941468*TemperatureEau))+ValueEtalon;

    Serial.print("PH: ");Serial.println(valPH);

    //Edit 29-Sep-2018
    lcd.setCursor (13,1);
    lcd.print("Ph:    ");
    lcd.setCursor (16,1);
    lcd.print(valPH,1);
    zunoSendReport(9);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : POMPE
 ******************************************/
void setPompe(byte value) {
  if((modePompe == 2) && (!fctGel)){
    
    if (value > 0) {               // if greater then zero
      etatPompe = true;
    } 
    else {                         // if equals zero
      etatPompe = false;
    }
  }
}

byte getPompe() {
  return (byte)etatPompe;
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : LUMIERE
 ******************************************/
void setLumiere(byte value) {
  if (value > 0) {               // if greater then zero
    etatLumiere = true;
  } 
  else {                         // if equals zero
    etatLumiere = false;
  }
}

byte getLumiere() {
  return (byte)etatLumiere;
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : MODE
 ******************************************/
byte getMode() {
  return (modePompe);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : DEMANDE HORAIRE
 ******************************************/
byte getHoraire() {
  return (byte)pinHoraire;
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : MODE ANTIGEL
 ******************************************/
byte getGel() {
  return (byte)fctGel;
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : TEMPERATURE LOCAL
 ******************************************/
int getTempLocal(){
  return (TemperatureLoc*10);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : TEMPERATURE EAU
 ******************************************/
int getTempEau(){
  return (TemperatureEau*10);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : ORP/REDOX
 ******************************************/
int getORP(){
  return (valORP);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : PH
 ******************************************/
int getPH(){
  return (int)(valPH*10);
}

/******************************************
 * FONCTIONS CANAUX ZWAVE : CHLORE PRODUCTION
 ******************************************/
byte getChloreProd(){
  return (byte)(etatChloreProd);
}
