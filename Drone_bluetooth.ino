#include <SoftwareSerial.h>
SoftwareSerial BlueSerial(2, 3); // les pin à utiliser pour le bluetooth sont 2=RX, 3=TX

//------------------------------------------------------------------------------------------
//   Variables globales non commune à tout projet Arduino EveryWhere
//
unsigned long LastTime;
int MotorVolant, MessageStep; 
bool Avance;

int PositionMessage;
char messageBLUE[20], MotorNo, CharTemp;

#define PIN_MOTEUR_1 4
#define PIN_MOTEUR_2 5 // ces pins recoient un signal analogique
#define PIN_VOLANT_D 6 // ces pins recoient un signal analogique
#define PIN_VOLANT_G 7
// gestion de la lumière
#define PIN_LIGHT    9 // ces pins recoient un signal analogique

#define VOLANT_CENTRE 444 // voiturenoire 600 // voiture rouge 700 //bull jaune 444 // 512 //404 ma petite caisse / 447 valeur petit lacroix
#define VOLANT_DROITE 180 // voiturenoire 10 // voiture rouge 135 //bull jaune 180 // 0 //200 ma petite caisse / 222 valeur petit lacroix 250
#define VOLANT_GAUCHE 746 // voiturenoire 1023 // voiture rouge 1023 //bull jaune 746 // 1023 //661 ma petite caisse / 676 valeur petit lacroix 611
#define VOLANT_JEUX 15 //20 valeur petit lacroix

//-----------
// gauge
#define PIN_Gauge 1
unsigned long TimeMesureGauge;
int ValGauge;
//
//   Fin des variables globales non commune à tout projet Arduino EveryWhere
//---------------------------------------------------------------------------------


void setup() {
  // la section ci dessous est nécessaire à Arduino EveryWhere

  ChercheBlueBauds();
  sendCommand("AT");
  sendCommand("AT+BAUD");
  sendCommand("AT+BAUD6");
  ChercheBlueBauds();
  sendCommand("AT+ROLE0");
  sendCommand("AT+UUID0xFFE0");
  sendCommand("AT+UUID");
  sendCommand("AT+CHAR");
  sendCommand("AT+CHAR0xFFE1");
  sendCommand("AT+CHAR");
  sendCommand("AT+NAMEAvion"); //BullJaune  VoitureRouge
  sendCommand("AT+NAME");
  sendCommand("AT+PIN123456");  
  
  // Fin de ce qui est nécessaire à Arduino EveryWhere
  //-----------------------------------------
  // quelques variables pour ce projet précis non nécessaire à Arduino EveryWhere
  Serial.begin(115200); // pour dialogue serie
  MoteurSetup();
  Avance=false;
  LastTime = millis();
  MessageStep = 0;
  TimeMesureGauge = 0; // pour la gauge
}

void sendCommand(const char * command){
  BlueSerial.println(command);
  //wait some time
  delay(100);
  
  char reply[100];
  int i = 0;
  while (BlueSerial.available()) {
    reply[i] = BlueSerial.read();
    i += 1;
  }
  //end the string
  reply[i] = '\0';
}
void ChercheBlueBauds(){

  if (TesteBauds( '1', 9600   ))return;  //9600 bah
  if (TesteBauds( '2', 2400   ))return;
  if (TesteBauds( '3', 4800   ))return;
  if (TesteBauds( '4', 9600   ))return;
  if (TesteBauds( '5', 19200  ))return;
  if (TesteBauds( '6', 38400  ))return;
  if (TesteBauds( '7', 57600  ))return;  
  if (TesteBauds( '8', 115200 ))return;  
}

bool TesteBauds( char mode, long bauds) {
  BlueSerial.begin(bauds);
  BlueSerial.println("AT");
  delay(50);
  
  
  if (BlueSerial.available()) {
  
    sendCommand("AT");
    return true;
  }
  else return false;
}

//-----------------------------------------------------------------------------
// à partir d'ici, tout est spécifique au projet Drone_bluetooth
// la partie commune à Arduino everywhere est au dessus.
// utilisez l'objet "BlueSerial" pour dialoguer avec le PC distant.
// les fonctions au dessus ne sont nécessaires qu'au démarage.


void loop() 
{  
    // décoder les messages moteurs et modifier les consignes

  // nouvelle méthode avec serial event
  if (MessageStep == 3)
  {   
    if ( MotorNo == '1' ) 
    {
      MotorVolant = ( 16 * ParseHex(messageBLUE[0])) + ParseHex(messageBLUE[1]);
      
      MotorVolant = 2*MotorVolant;
      
      MotorVolant = MotorVolant-254;
      Serial.print("\nMot :1 Vit :"); // pour dialogue serie
      Serial.println(MotorVolant); // pour dialogue serie
  
      MoteurVitesse ( MotorVolant);
    } 
    // M2:7F>   M1:7F>
    
    if ( MotorNo == '2' ) 
    {     
      MotorVolant = ( 16 * ParseHex(messageBLUE[0])) + ParseHex(messageBLUE[1]);

      MotorVolant = (int) (((long)MotorVolant * (long) (VOLANT_GAUCHE - VOLANT_DROITE)) / 256) ;

      MotorVolant = MotorVolant + VOLANT_DROITE ;

      Serial.print("\nMot :2 Vit :"); // pour dialogue serie
      Serial.println(MotorVolant); // pour dialogue serie
      ChangeLaConsigne ( MotorVolant );
    }

    // gestion de la lumière   
    if ( MotorNo == '3' ) 
    {
      MotorVolant = ( 16 * ParseHex(messageBLUE[0])) + ParseHex(messageBLUE[1]);

      Serial.print("\nMot :3 Vit :"); // pour dialogue serie
      Serial.println(MotorVolant); // pour dialogue serie
      
      analogWrite(PIN_LIGHT, 255 - MotorVolant );  // j'inverse la puissance 
                                      // car le transistor inverse la tension
    } 
   
    MessageStep = 0;
  }

  GestionDuMoteurVolant();
  GestionGauge();
  serialBLUE();
}

void serialBLUE() {
  while (BlueSerial.available()) {
    // get the new byte:
    CharTemp = (char)BlueSerial.read();
    Serial.write(CharTemp); // pour dialogue serie
        
    if ( MessageStep >= 1 )
    {
      if ( MessageStep >= 2 )
      {
        if ( CharTemp == '>' )
        {
          // cette étape sert à signaler que le buffer est plein
          MessageStep = 3;
          return;
        }
        else
        {
          // <<<etape 2>>> on remplis le buffer
          messageBLUE[PositionMessage] = CharTemp;
          PositionMessage++;            
        }
      }
      else 
      {
        if ( CharTemp == ':' )
        {
          PositionMessage = 0;
          MessageStep = 2;
        }
        else
        {
          // <<<etape 1>>> si c'est pas ':' c'est le n° de moteur
          MotorNo = CharTemp;          
        }
      }
    }
    else
    {
      // <<<étape 0>>> on attends le caractere M
      if ( CharTemp == 'M' )
      {
        MessageStep = 1;
      }
    }
  }
}
//-------------------------------------------------------------


int TheNumber;
int ParseHex ( char TheHexCode )
{
  TheNumber = 0;
  if ( TheHexCode >= '0' )
  {
    if ( TheHexCode > '9' )
    {
      if ( TheHexCode >= 'A' && TheHexCode < 'G'  )
      {
        TheNumber = TheHexCode + 10 - 'A';
      }
     if ( TheHexCode >= 'a' && TheHexCode < 'g'  )
      {
        TheNumber = TheHexCode + 10 - 'a';
      }
    }
    else 
    {
      TheNumber = TheHexCode - '0';
    }
    
  }

   return TheNumber;
}


//#########################################################################################
//
//  Fonctions pour les moteurs
//
float PuissanceVolant, ConsignePourVolant;

//--------------------------------------------------------------------------------------
//  initialise les moteurs
void MoteurSetup ()
{
  pinMode(PIN_MOTEUR_1, OUTPUT);
  pinMode(PIN_MOTEUR_2, OUTPUT);
  pinMode(PIN_VOLANT_G, OUTPUT);
  pinMode(PIN_VOLANT_D, OUTPUT);
  // gestion de la lumière
  pinMode(PIN_LIGHT, OUTPUT);
  
  MoteurStop();
  ConsignePourVolant = VOLANT_CENTRE;  // position
  analogWrite(PIN_LIGHT, 255 ); // phares éteinds
}

//--------------------------------------------------------------------------------------
//  Fonctions pour le Moteur Qui Gere le déplacement

void MoteurVitesse ( int Rapiditee )
{
  // M1:7F>  M2:7F>
  if ( Rapiditee == 0 )
  {
    MoteurStop();
  }
  else
  {
    if ( Rapiditee  > 0 )
    {
      digitalWrite(PIN_MOTEUR_1, HIGH);
      analogWrite(PIN_MOTEUR_2, 256-Rapiditee );
    }
    else
    {
      digitalWrite(PIN_MOTEUR_1, LOW);
      analogWrite(PIN_MOTEUR_2, -Rapiditee );
    }
  }
}

void MoteurStop()
{
  digitalWrite(PIN_MOTEUR_1, LOW);
  digitalWrite(PIN_MOTEUR_2, LOW);
}

//--------------------------------------------------------------------------------------
//  Fonctions pour le Moteur Qui Gere le Volant
//
// Cette fonction doit être appellé très souvent    Proportionel_VOLANT PuissanceVolant
float proportional = 10.000;    //initial value
#define   MOTOR_LEVEL_NOISY   40.0 

void GestionDuMoteurVolant()
{   
  float error = (float)analogRead(A0);
  error = error - ConsignePourVolant; 
  
  float pTerm_motor_R = proportional * error;
  
  PuissanceVolant = constrain((pTerm_motor_R ), -255, 255);
  
  
  if (PuissanceVolant > 0)
  {   
    // si trop à gauche, tourne à droite
      digitalWrite(PIN_VOLANT_G, LOW);
      if (PuissanceVolant<MOTOR_LEVEL_NOISY)PuissanceVolant=0;
      analogWrite(PIN_VOLANT_D, (int)PuissanceVolant );
  }
  else
  {
    // si trop à droite, tourne à gauche
    digitalWrite(PIN_VOLANT_G, HIGH);
    if (PuissanceVolant>-MOTOR_LEVEL_NOISY)PuissanceVolant=0;    
    analogWrite(PIN_VOLANT_D, 255+(int)PuissanceVolant );
  }
}

/*
#define   GUARD_MOTOR_GAIN   255.0     

float K_motor_    = 1;
float proportional = 10.000;    //initial value
float integral   = 0.0; //1.000;
float derivative   = 0.0; //1.500;
float integrated_motor_error = 0;
float mean_dTerm_motor_R = 0;
float last_motor_error    = 0;
int timeout=0;

void GestionDuMoteurVolant()
{   
  float error = (float)analogRead(A0);
  error = error - ConsignePourVolant; 
  
  float pTerm_motor_R = proportional * error;
  
  integrated_motor_error = constrain(integrated_motor_error + error, -GUARD_MOTOR_GAIN, GUARD_MOTOR_GAIN);                                   
  float iTerm_motor_R =  integral * integrated_motor_error;

  mean_dTerm_motor_R = (mean_dTerm_motor_R * 49 +(error - last_motor_error))/50 ;
  float dTerm_motor_R = derivative * mean_dTerm_motor_R;                            
  last_motor_error = error;
  PuissanceVolant = constrain(K_motor_*(pTerm_motor_R + iTerm_motor_R + dTerm_motor_R), -255, 255);
  
  //BlueSerial.print(" pow ");
  if (timeout>100)
  {
    timeout=0;
    */
    /*BlueSerial.print(ConsignePourVolant,1);
    BlueSerial.print(" err ");
    BlueSerial.print(error,1);*/ /*
    BlueSerial.print(" p ");
    BlueSerial.println(pTerm_motor_R,0);
    BlueSerial.print(" i ");
    BlueSerial.println(iTerm_motor_R,0);
    BlueSerial.print(" d ");
    BlueSerial.println(dTerm_motor_R,3);
  }
  timeout++;
  //delay(1000);
    
  
  if (PuissanceVolant > 0)
  {   
    // si trop à gauche, tourne à droite
      digitalWrite(PIN_VOLANT_G, LOW);
    
      analogWrite(PIN_VOLANT_D, (int)PuissanceVolant );
  }
  else
  {
    // si trop à droite, tourne à gauche
    digitalWrite(PIN_VOLANT_G, HIGH);
    
    analogWrite(PIN_VOLANT_D, 255+(int)PuissanceVolant );
  }
}*/
/* 
int DifferenceConsigneMesure;
void GestionDuMoteurVolantOld()
{   
  DifferenceConsigneMesure = analogRead(A0) - ConsignePourVolant;
  
  if (DifferenceConsigneMesure > 0)
  {  // si trop à gauche, tourne à droite
    digitalWrite(PIN_VOLANT_G, LOW);
  PuissanceVolant = Proportionel_VOLANT * (float)DifferenceConsigneMesure;
  
  if ( PuissanceVolant > 255 ) DifferenceConsigneMesure = 255;
  else 
  { 
    if ( PuissanceVolant > VOLANT_JEUX ) DifferenceConsigneMesure = (int)PuissanceVolant;
    else DifferenceConsigneMesure=0;
  }
    analogWrite(PIN_VOLANT_D, DifferenceConsigneMesure );
  }
  else
  {
    // si trop à droite, tourne à gauche
    digitalWrite(PIN_VOLANT_G, HIGH);
  PuissanceVolant = -Proportionel_VOLANT * (float)DifferenceConsigneMesure;
  
  if ( PuissanceVolant > 255 ) DifferenceConsigneMesure = 255;
  else 
  { 
    if ( PuissanceVolant > VOLANT_JEUX ) DifferenceConsigneMesure = (int)PuissanceVolant;
    else DifferenceConsigneMesure=0;
  }
    analogWrite(PIN_VOLANT_D, 255-DifferenceConsigneMesure );
  }
}
*/

// cette fonction sert surtout si on veut tourner très précisément le volant
void ChangeLaConsigne (int NouvelleConsigne)
{  
  // ne dépassons pas les limites
  if ( NouvelleConsigne > VOLANT_GAUCHE ) NouvelleConsigne = VOLANT_GAUCHE;
  if ( NouvelleConsigne < VOLANT_DROITE ) NouvelleConsigne = VOLANT_DROITE;  
  
  ConsignePourVolant = (float)NouvelleConsigne;

}

void NePasTourner ()
{
  ConsignePourVolant = VOLANT_CENTRE;
}

//--------------------------------------------------
void GestionGauge()
{
  if ( (millis() - TimeMesureGauge)>1000)
  {
    TimeMesureGauge = millis();
    ValGauge = analogRead(PIN_Gauge);
    BlueSerial.print("V1:");
    BlueSerial.print(ValGauge);
    BlueSerial.print(">");
  }
}
