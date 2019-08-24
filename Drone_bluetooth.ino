#include <SoftwareSerial.h>
SoftwareSerial BlueSerial(2, 3); // RX, TX

unsigned long mesure, LastTime;
int Vitesse, MotorVolant, MessageStep;
bool Avance;

int PositionMessage;
char messageBLUE[20], MotorNo, CharTemp;

#define PIN_MOTEUR_1 4
#define PIN_MOTEUR_2 5 // ces pins recoient un signal analogique
#define PIN_VOLANT_D 6 // ces pins recoient un signal analogique
#define PIN_VOLANT_G 7
// gestion de la lumière
#define PIN_LIGHT    9 // ces pins recoient un signal analogique

#define VOLANT_CENTRE 469 // 512 //404 ma petite caisse / 447 valeur petit lacroix
#define VOLANT_DROITE 244 // 0 //200 ma petite caisse / 222 valeur petit lacroix 250
#define VOLANT_GAUCHE 725 // 1023 //661 ma petite caisse / 676 valeur petit lacroix 611
#define VOLANT_JEUX 15 //20 valeur petit lacroix
#define Proportionel_VOLANT 5.23 // pour rendre le volant + ou moin réactif

//-----------
// gauge
#define PIN_Gauge 1
unsigned long TimeMesureGauge;
int ValGauge;

void setup() {
  // put your setup code here, to run once:
  MoteurSetup();
  
  // initialise le port USB  ( les 3 lignes ci dessous )
  // BlueSerial.begin(9600);

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
  sendCommand("AT+NAMEbluino");
  sendCommand("AT+NAME");
  sendCommand("AT+PIN123456");  
  
  // quelques variables pour ce projet
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
  // un jours peut être si on veut constater le retour blue tooth
  //Serial.print(reply);
  //Serial.println("Reply end");
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
// messageBLUE[20]  PositionMessage   MotorNo
//------------------------------------------
void serialBLUE() {
  while (BlueSerial.available()) {
    // get the new byte:
    CharTemp = (char)BlueSerial.read();
        
    if ( MessageStep >= 1 )
    {
      if ( MessageStep >= 2 )
      {
        if ( CharTemp == '>' )
        {
          // cette étape sert à signaler que le buffer est plein
          MessageStep = 3;
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
      
      MoteurVitesse ( MotorVolant);
    } 
    // M2:7F>   M1:7F>
    
    if ( MotorNo == '2' ) 
    {     
      MotorVolant = ( 16 * ParseHex(messageBLUE[0])) + ParseHex(messageBLUE[1]);

      MotorVolant = (int) (((long)MotorVolant * (long) (VOLANT_GAUCHE - VOLANT_DROITE)) / 256) ;

      MotorVolant = MotorVolant + VOLANT_DROITE ;

      ChangeLaConsigne ( MotorVolant );
    }

    // gestion de la lumière   
    if ( MotorNo == '3' ) 
    {
      MotorVolant = ( 16 * ParseHex(messageBLUE[0])) + ParseHex(messageBLUE[1]);

      analogWrite(PIN_LIGHT, 255 - MotorVolant );  // j'inverse la puissance 
                                      // car le transistor inverse la tension
    } 
   
    MessageStep = 0;
  }

  GestionDuMoteurVolant();
  GestionGauge();
  serialBLUE();
}



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

int ConsignePourVolant, DifferenceConsigneMesure;
float PuissanceVolant;

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

void GestionDuMoteurVolant()
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

// cette fonction sert surtout si on veut tourner très précisément le volant
void ChangeLaConsigne (int NouvelleConsigne)
{  
  // M2:7F>
  
  // ne dépassons pas les limites
  if ( NouvelleConsigne > VOLANT_GAUCHE ) NouvelleConsigne = VOLANT_GAUCHE;
  if ( NouvelleConsigne < VOLANT_DROITE ) NouvelleConsigne = VOLANT_DROITE;  
  
  ConsignePourVolant = NouvelleConsigne;

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







