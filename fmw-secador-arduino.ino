#include <DallasTemperature.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <Nextion.h>

//https://dl.espressif.com/dl/package_esp32_index.json

#define OUT_QUEIMADOR       12
#define OUT_BUZINA          13
#define LED_MASSA_QUENTE     5
#define LED_MASSA_FRIO       4
#define LED_ENTRADA_QUENTE   0
#define LED_ENTRADA_FRIO     2
#define LED_CONEXAO         18
#define SENSORT             15

NexPage page0 = NexPage(0, 0, "page0");                 //--> Tela Central

NexDSButton Mudo       = NexDSButton(0, 15, "bt0");                        //--> Mudo/som
NexDSButton PalhaLenha = NexDSButton(0, 16, "bt1");                        //--> Palha/lenha

NexNumber TemperaturaT  = NexNumber(0, 13, "n4");                          //--> Temperatura da Turbina        (Write)
NexNumber TemperaturaM  = NexNumber(0, 14, "n5");                          //--> Temperatura da Massa          (Write)

NexNumber TemperaturaST = NexNumber(0, 9, "n0");                           //--> Temperatura máxima da Turbina (Read)
NexNumber TemperaturaMT = NexNumber(0, 10, "n1");                          //--> Temperatura mínima da Turbina (Read)
NexNumber TemperaturaSM = NexNumber(0, 11, "n2");                          //--> Temperatura máxima da Massa   (Read)
NexNumber TemperaturaMM = NexNumber(0, 12, "n3");                          //--> Temperatura mínima da Massa   (Read)

NexPage page1 = NexPage(1, 0, "page1");                 //--> Tela <120
NexPage page2 = NexPage(2, 0, "page2");                 //--> Tela min>max
NexPage page3 = NexPage(3, 0, "page3");                 //--> Tela massa acima
NexPage page4 = NexPage(4, 0, "page4");                 //--> Tela entrada acima
NexPage page5 = NexPage(5, 0, "page5");                 //--> Tela entrada abaixo
NexPage page6 = NexPage(6, 0, "page6");                 //--> Tela massa abaixo

#define ssid     "CT_01"                                                   //--> NOME DO WIFI
#define password "AusyxSolucoes01"                                           //--> SENHA DO WIFI

OneWire oneWire(SENSORT);
DallasTemperature sensors(&oneWire);

WiFiServer server(80);

uint32_t ST = 0;                                                           //--> Temperatura máxima turbina
uint32_t MT = 0;                                                           //--> Temperatura mínima turbina
uint32_t SM = 0;                                                           //--> Temperatura máxima massa
uint32_t MM = 0;                                                           //--> Temperatura mínima massa

uint32_t S1;                                                               //--> Status do botão mudo
uint32_t S2;                                                               //--> Status do botão modo

float SensorTemperaturaT = 0;
float SensorTemperaturaM = 0;

int i = 0;
int y = 0;
int z = 0;

void setup()
{
  nexInit();

  pinMode(OUT_QUEIMADOR, OUTPUT);
  pinMode(OUT_BUZINA, OUTPUT);
  pinMode(LED_MASSA_QUENTE, OUTPUT);
  pinMode(LED_MASSA_FRIO, OUTPUT);
  pinMode(LED_ENTRADA_QUENTE, OUTPUT);
  pinMode(LED_ENTRADA_FRIO, OUTPUT);
  pinMode(LED_CONEXAO, OUTPUT);

  digitalWrite(OUT_QUEIMADOR, HIGH);
  digitalWrite(OUT_BUZINA, HIGH);

  digitalWrite(LED_MASSA_QUENTE,HIGH);
  digitalWrite(LED_MASSA_FRIO,HIGH);
  digitalWrite(LED_ENTRADA_QUENTE,HIGH);
  digitalWrite(LED_ENTRADA_FRIO,HIGH);

  IPAddress staticIP(192, 168, 4, 2);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.softAP(ssid, password, 2, 0);

  sensors.begin();
  server.begin();
  

  EEPROM.begin(6);

  ST = EEPROM.read(0);
  MT = EEPROM.read(1);
  SM = EEPROM.read(2);
  MM = EEPROM.read(3);
  S1 = EEPROM.read(4);
  S2 = EEPROM.read(5);

  TemperaturaST.setValue(ST);
  TemperaturaSM.setValue(SM);
  TemperaturaMM.setValue(MM);
  TemperaturaMT.setValue(MT);
  Mudo.setValue(S1);
  PalhaLenha.setValue(S2);

  //  ST = 0;
  //  MT = 0;
  //  SM = 0;
  //  MM = 0;

  delay(500);
}

void TemperaturaMassa()
{
  WiFiClient cliente = server.available();
  if (!cliente)
  {
    Serial.println("!cleint");
    digitalWrite(LED_CONEXAO, LOW);
    SensorTemperaturaM = 0;
    return;
  }
  Serial.println("cl!!!!eint");
  digitalWrite(LED_CONEXAO, HIGH);
  while (!cliente.available())
  {
    if (cliente) return;
  }
  
  String req = cliente.readStringUntil('\r');
  req = req.substring(req.indexOf("/") + 1, req.indexOf("HTTP") - 1);
  cliente.flush();

  if (req.indexOf("TM") != -1)
  {
    req.remove(0, 3);
    float reqc = req.toFloat();
    SensorTemperaturaM = reqc;
    SensorTemperaturaM = SensorTemperaturaM / 100;
    if (SensorTemperaturaM < 0) SensorTemperaturaM = 0;
  }
  else
  {
    cliente.print("Invalid Request");
    cliente.flush();
    cliente.stop();
    return;
  }
  cliente.flush();
}

void TemperaturaTurbina()
{
  sensors.requestTemperatures();
  SensorTemperaturaT = sensors.getTempCByIndex(0);
  if (SensorTemperaturaT < 0) SensorTemperaturaT = 0;
}

void loop()
{
  Serial.println("Loop!!");
  Mudo.getValue(&S1);
  PalhaLenha.getValue(&S2);

  TemperaturaTurbina();
  TemperaturaMassa();

  TemperaturaT.setValue(SensorTemperaturaT);
  TemperaturaM.setValue(SensorTemperaturaM);

  TemperaturaST.getValue(&ST);
  TemperaturaMT.getValue(&MT);
  TemperaturaSM.getValue(&SM);
  TemperaturaMM.getValue(&MM);



  EEPROM.write(0, ST);
  EEPROM.write(1, MT);
  EEPROM.write(2, SM);
  EEPROM.write(3, MM);
  EEPROM.write(4, S1);
  EEPROM.write(5, S2);
  EEPROM.commit();

  i = !i;
  
  if (SensorTemperaturaT > ST)
  {
    digitalWrite(LED_ENTRADA_FRIO,HIGH); //coloquei
    digitalWrite(LED_ENTRADA_QUENTE, LOW);
    y = 1;
    if (S2 == 0) 
    {
      // Desliga o queimador 
      digitalWrite(OUT_QUEIMADOR, HIGH);
      if (S1 == 0 && i == 0 && SensorTemperaturaT >= (ST + 7))
      {
        page4.show();
        digitalWrite(OUT_BUZINA, LOW);
        delay(4000);
        digitalWrite(LED_ENTRADA_FRIO,HIGH); //coloquei
        page0.show();

        //Seta os valores na pagina novamente, se nao setasse a pagina seria carregada tudo zerado
        TemperaturaST.setValue(ST);
        TemperaturaMT.setValue(MT);
        TemperaturaSM.setValue(SM);
        TemperaturaMM.setValue(MM);
        Mudo.setValue(S1);
        PalhaLenha.setValue(S2);
        TemperaturaT.setValue(SensorTemperaturaT);
        TemperaturaM.setValue(SensorTemperaturaM);
        delay(2000);        
      }
      else digitalWrite(OUT_BUZINA, HIGH);
    }
    else
    {
      if (S1 == 0 && i == 0 && SensorTemperaturaT >= (ST + 7))
      {
        page4.show();
        digitalWrite(OUT_BUZINA, LOW);
        delay(4000);
        page0.show();
        TemperaturaST.setValue(ST);
        TemperaturaMT.setValue(MT);
        TemperaturaSM.setValue(SM);
        TemperaturaMM.setValue(MM);
        Mudo.setValue(S1);
        PalhaLenha.setValue(S2);                
        TemperaturaT.setValue(SensorTemperaturaT);
        TemperaturaM.setValue(SensorTemperaturaM);
        delay(2000);
      }
      else digitalWrite(OUT_BUZINA, HIGH);
    }
  }
  else
  {
    y = 0;
    digitalWrite(OUT_BUZINA, HIGH);
    digitalWrite(LED_ENTRADA_QUENTE, HIGH);
    if (S2 == 0 && z == 0) digitalWrite(OUT_QUEIMADOR, LOW);
    else digitalWrite(OUT_QUEIMADOR, HIGH);
  }
  if (SensorTemperaturaM > SM)
  {
    digitalWrite(LED_MASSA_FRIO, HIGH);
    digitalWrite(LED_MASSA_QUENTE, LOW);
    if (S2 == 0)
    {
      digitalWrite(OUT_QUEIMADOR, HIGH);
      if (S1 == 0 && i == 0 && SensorTemperaturaM >= (SM + 7))
      {
        page3.show();
        digitalWrite(OUT_BUZINA, LOW);
        delay(4000);
        digitalWrite(LED_MASSA_FRIO, HIGH);
        page0.show();
        TemperaturaST.setValue(ST);
        TemperaturaMT.setValue(MT);
        TemperaturaSM.setValue(SM);
        TemperaturaMM.setValue(MM);
        Mudo.setValue(S1);
        PalhaLenha.setValue(S2);      
        TemperaturaT.setValue(SensorTemperaturaT);
        TemperaturaM.setValue(SensorTemperaturaM);
        delay(2000);
      }
      else digitalWrite(OUT_BUZINA, HIGH);
    }
    else
    {
      if (S1 == 0 && i == 0  && SensorTemperaturaM >= (SM + 7))
      {
        page3.show();
        digitalWrite(OUT_BUZINA, LOW);
        delay(4000);
        page0.show();
        Mudo.setValue(S1);
        PalhaLenha.setValue(S2);                
        TemperaturaT.setValue(SensorTemperaturaT);
        TemperaturaM.setValue(SensorTemperaturaM);
        delay(2000);
      }
      else digitalWrite(OUT_BUZINA, HIGH);
    }
  }
  else
  {
    z = 0;
    digitalWrite(OUT_BUZINA, HIGH);
    digitalWrite(LED_MASSA_QUENTE, HIGH);
    if (S2 == 0 && y == 0) digitalWrite(OUT_QUEIMADOR, LOW);
    else digitalWrite(OUT_QUEIMADOR, HIGH);
  }

  
  if (SensorTemperaturaT < MT)
  {
    digitalWrite(LED_ENTRADA_FRIO, LOW);
    if (S1 == 0 && i == 0 && SensorTemperaturaT <= (MT - 10))
    {
      page5.show();
      digitalWrite(OUT_BUZINA, LOW);
      delay(4000);
      page0.show();
      TemperaturaST.setValue(ST);
      TemperaturaMT.setValue(MT);
      TemperaturaSM.setValue(SM);
      TemperaturaMM.setValue(MM);
      Mudo.setValue(S1);
      PalhaLenha.setValue(S2);            
      TemperaturaT.setValue(SensorTemperaturaT);
      TemperaturaM.setValue(SensorTemperaturaM);
      delay(2000);
    }
    else digitalWrite(OUT_BUZINA, HIGH);
  }
  else digitalWrite(LED_ENTRADA_FRIO, HIGH);
  if (SensorTemperaturaM < MM)
  {
    digitalWrite(LED_MASSA_FRIO, LOW);
    if (S1 == 0 && i == 0  && SensorTemperaturaT <= (MM - 10))
    {
      page6.show();
      digitalWrite(OUT_BUZINA, LOW);
      delay(4000);
      page0.show();
      TemperaturaST.setValue(ST);
      TemperaturaMT.setValue(MT);
      TemperaturaSM.setValue(SM);
      TemperaturaMM.setValue(MM);
      Mudo.setValue(S1);
      PalhaLenha.setValue(S2);          
      TemperaturaT.setValue(SensorTemperaturaT);
      TemperaturaM.setValue(SensorTemperaturaM);
      delay(2000);
    }
    else digitalWrite(OUT_BUZINA, HIGH);
  }
  else digitalWrite(LED_MASSA_FRIO, HIGH);

  Serial.print("S1:");
  Serial.println(S1);
  Serial.print("S2:");
  Serial.println(S2);
  Serial.print("ST:");
  Serial.println(ST);
  Serial.print("MT:");
  Serial.println(MT);
  Serial.print("SM:");
  Serial.println(SM);
  Serial.print("MM:");
  Serial.println(MM);

}
