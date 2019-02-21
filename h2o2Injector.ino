#include <ESP8266WiFi.h>
//#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266httpUpdate.h>
//#include <DNSServer.h>
#include <TimeLib.h>
//#include <Wire.h>
//#include <SPI.h>
#include <EEPROM.h>
#include "SSD1306.h"
//#include "SH1106.h"
#include "SH1106Wire.h"
#include "ds3231.h"

void handleRoot();   // ide sometimes compiles and sometimes does not ????
void handleSetup();
void handleNotFound();
void i2cScan();

const byte MOTOR1 = D7 ;       // PWM
const byte BEEPER = D8 ;       // pezo
const byte SCOPE_PIN = D5 ;    // for cycle timer measure
const byte PROG_BASE = 40 ;    // 

#define BUFF_MAX 32
#define TIMER_MAX 3600000
#define TIMER_MIN 250
#define CYCLE_COUNT_MAX 1000000
#define PWM_INC_MIN 0
#define PWM_INC_MAX 50
#define PUMPED_QTY_MAX 20000.0
#define PUMP_ML_PER_SEC_MIN 50.0
#define PUMP_ML_PER_SEC_MAX 10000.0

#define MAX_PUMPS 2

//SSD1306 display(0x3c, 5, 4);   // GPIO 5 = D1, GPIO 4 = D2   - onboard display 0.96" 
SH1106Wire display(0x3c, 4, 5);   // arse about ??? GPIO 5 = D1, GPIO 4 = D2  -- external ones 1.3"

const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // buffer to hold incoming and outgoing packets

typedef struct __attribute__((__packed__)) {            // eeprom stuff
  long lNodeAddress ;
  char TimeServer[24] ;
  char nssid[32] ;
  char npassword[16] ;
  char cssid[32] ;
  char cpassword[16] ;
  char NodeName[16] ;
  float fTimeZone ;  
  IPAddress MyIP  ;                 // main
  IPAddress MyIPC  ;                
  unsigned int localPort ;          // local port to listen for NTP UDP packets
  unsigned int localPortCtrl ;      // local port to listen for Control UDP packets
  unsigned int RemotePortCtrl ;     // local port to listen for Control UDP packets
  unsigned int uiThorPort ;
  unsigned int uiModbusPort ;       // modbus port
  bool bNWG ; 
}network_stuff_t ;                        // network control

typedef struct __attribute__((__packed__)) {            // volitile stuff for RTC if present
  bool bHasRTC ;
  byte rtc_sec ;
  byte rtc_min ;
  byte rtc_hour ;
  float rtc_temp ;
  uint8_t rtc_status ;
  bool bDoTimeUpdate ;  
  struct ts tc;
  tmElements_t tm;
} rtc_stuff_t ;                   

typedef struct __attribute__((__packed__)) {            // eeprom stuff
  long lPrime ;                // 0  modbus offset
  long lOnTime ;               // 2
  long lOffTime ;              // 4
  long lWebOnTime ;            // 6
  long lOnCounter ;            // 8
  long lOffCounter ;           // 10
  long lCyclesCounter ;        // 12
  long lCycles ;               // 14
  float dblMLPerSecond ;       // 16 
  float dblQty ;               // 18 
  float dblCurrentQty ;        // 20
  long lOnOff ;                // 22
  long lTimePrev ;             // 24  
  long lTimePrev2 ;            // 26
  long PWM_inc ;               // 28 
  time_t AutoOff_t ;           // 30  yep its a long
  int  iFinish ;               // 31
  int  iCurPos ;               // 32
  int  iMatchBit ;             // 33
  int  iMatchSlot ;            // 34
  int  iOut ;                  // 35
  bool bState ;                // 36 done in one
  bool bPrime ;                // 36
} pump_stuff_t ;                   

typedef struct __attribute__((__packed__)) {            // eeprom stuff
  uint8_t net_id ; 
  uint8_t fc ;   
  uint16_t address ;   
  uint16_t wordcount ; 
  uint16_t remotePort ;
  IPAddress remoteIP ;
} modbus_stuff_t ;                   


rtc_stuff_t      rtc ;             // all volitile stuff
network_stuff_t  nwc ;             // saved in eeprom  Network Control
pump_stuff_t     pcs ;             // saved in eprom   Pump Control System
modbus_stuff_t   MB ;

/* Set these to your desired credentials. */
const char *ssid = "Injector";
const char *password = "password";
const char *host = "injector";
char Toleo[10] = {"Ver 3.1A\0"}  ;

long lScanCtr = 0 ;
long lScanLast = 0 ;


char buff[BUFF_MAX]; 
long lDisplayOptions = 1 ;
String LastPacket ;
time_t PacketTime ;
//long lWebOnTime ;
//IPAddress MyIP(192,168,2,110) ;

ESP8266WebServer server(80);
//DNSServer dnsServer;
ESP8266WebServer OTAWebServer(81);
ESP8266HTTPUpdateServer OTAWebUpdater;

WiFiUDP ctrludp;
WiFiUDP ntpudp;
WiFiUDP modbusudp;

void FloatToModbusWords(float src_value , uint16_t * dest_lo , uint16_t * dest_hi ) {
  uint16_t tempdata[2] ;
  float *tf ;
  tf = (float * )&tempdata[0]  ;
  *tf = src_value ;
  *dest_lo = tempdata[1] ;
  *dest_hi = tempdata[0] ;
}
float FloatFromModbusWords( uint16_t dest_lo , uint16_t dest_hi ) {
  uint16_t tempdata[2] ;
  float *tf ;
  tf = (float * )&tempdata[0]  ;
  tempdata[1] = dest_lo ;
  tempdata[0] = dest_hi  ;
  return (*tf) ;
}
int NumberOK (float target) {
  int tmp = 0 ;
  tmp = isnan(target);
  if ( tmp != 1 ) {
    tmp = isinf(target);
  }
  return (tmp);
}

void LoadParamsFromEEPROM(bool bLoad){
long eeAddress ;  
  if ( bLoad ) {
    eeAddress = 0 ;
    EEPROM.get(eeAddress,lDisplayOptions);
    eeAddress = PROG_BASE ;
    EEPROM.get(eeAddress,pcs);
    eeAddress += sizeof(pcs) ;
    EEPROM.get(eeAddress,nwc);
    eeAddress += sizeof(nwc) ;  
      
  }else{
    eeAddress = 0 ;
    EEPROM.put(eeAddress,lDisplayOptions);
    eeAddress = PROG_BASE ;
    EEPROM.put(eeAddress,pcs);
    eeAddress += sizeof(pcs) ;
    EEPROM.put(eeAddress,nwc);
    eeAddress += sizeof(nwc) ;        
    EEPROM.commit();  // save changes in one go ???
  }
}

void BackInTheBoxMemory(){
  sprintf( nwc.TimeServer , "au.pool.ntp.org\0" ) ;
  sprintf(nwc.NodeName,"Control_%08X\0",ESP.getChipId());  
  nwc.localPort = 2390 ;                                // local port to listen for NTP UDP packets
  nwc.localPortCtrl = 8666 ;                            // local port to listen for Control UDP packets
  nwc.RemotePortCtrl = 8664 ;                           // local port to listen for Control UDP packets
  nwc.uiThorPort = 8056 ;
  sprintf( nwc.nssid,"lodge\0" ) ;
  sprintf( nwc.npassword,"\0" ) ;
  sprintf(nwc.cssid,"Control_%08X\0",ESP.getChipId());  
//  sprintf( nwc.cssid,"Injector\0" ) ;
  sprintf( nwc.cpassword,"\0" ) ;
  nwc.MyIP = IPAddress(192,168,1,111) ;                                  // main
  nwc.MyIPC = IPAddress(192, 168, (5 +(ESP.getChipId() & 0x7f )) , 1) ;  // configuration              
  nwc.uiThorPort = 8056 ;
  nwc.fTimeZone = 10.0 ;
  nwc.lNodeAddress = 0 ;
  nwc.uiModbusPort = 502 ;
  
  pcs.iMatchBit = 0 ;
  pcs.iMatchSlot = 9 ;
  pcs.lOnTime = 4000 ;                     // On time in ms
  pcs.lOffTime = 26000 ;                   // Off time in ms
  pcs.lCycles  = 32000 ;                   // maximum no of cycles        
  pcs.dblMLPerSecond = 100.0 ;             // pump rate
  pcs.dblQty = 0 ;                         // Total Qty Required Litres
  pcs.lPrime = 15000 ;                     // On time in ms
  pcs.lOnOff = 0 ;                         // start and run
  pcs.PWM_inc = 0 ;                        // pwm increment 0 is no PWM
  pcs.iMatchBit = 31 ;
  pcs.iMatchSlot = 9 ;
  pcs.iFinish = 0 ;

  sprintf(nwc.npassword,"XXXXXXXX\0");
  sprintf(nwc.nssid,"XXXXXXXX\0");
  
}

//###########################################  SETUP  ##########################################################
void setup() {
int j ;
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  Serial.print("\nChip ID ");
  Serial.println(ESP.getChipId(), HEX);
  rtc.bDoTimeUpdate = false ;
  rtc.bHasRTC = false ;
  nwc.bNWG = false ;
  EEPROM.begin(2000);  
  LoadParamsFromEEPROM(true);
  
  display.init();
  if (( lDisplayOptions & 0x01 ) != 0 ) {  // if bit one on then flip the display
    display.flipScreenVertically();
  }

  /* show start screen */
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);  
  display.setFont(ArialMT_Plain_16);
  display.drawString(63, 0, "Injection");
  display.drawString(63, 16, "Controler");
  display.setFont(ArialMT_Plain_10);
  display.drawString(63, 34, "Copyright (c) 2019");
  display.drawString(63, 44, "Dougal Plummer");
  display.drawString(63, 54, String(Toleo));
  display.display();
  if (( nwc.uiThorPort == 0 ) || ( nwc.uiThorPort == 0xffff ) || (( pcs.lOnTime == 0 )  && ( pcs.lOffTime == 0 )) || (( pcs.lOnTime == 0xffffffff )  && ( pcs.lOffTime == 0xffffffff ))) {
    BackInTheBoxMemory();         // load defaults if blank memory detected but dont save user can still restore from eeprom
    Serial.println("Loading memory defaults...");
  }
//  BackInTheBoxMemory();         // load defaults if blank memory detected but dont save user can still restore from eeprom

  delay(2000);
  WiFi.disconnect();
  Serial.println("Configuring soft access point...");
//  WiFi.mode(WIFI_AP_STA);  // we are having our cake and eating it eee har
  if ( nwc.cssid[0] == 0 || nwc.cssid[1] == 0 ){   // pick a default setup ssid if none
    sprintf(nwc.cssid,"Configure_%08X\0",ESP.getChipId());
  }
  WiFi.softAPConfig(nwc.MyIPC,nwc.MyIPC,IPAddress (255, 255, 255 , 0));  
  sprintf(nwc.cpassword,"\0");
  Serial.println("Starting access point...");
  Serial.print("SSID: ");
  Serial.println(nwc.cssid);
  Serial.print("Password: >");
  Serial.print(nwc.cpassword);
  Serial.println("<");
  if ( nwc.cpassword[0] == 0 ){
    WiFi.softAP((char*)nwc.cssid);                   // no passowrd
  }else{
    WiFi.softAP((char*)nwc.cssid,(char*) nwc.cpassword);
  }

  nwc.MyIPC = WiFi.softAPIP();  // get back the address to verify what happened
  Serial.print("Soft AP IP address: ");
  snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", nwc.MyIPC[0],nwc.MyIPC[1],nwc.MyIPC[2],nwc.MyIPC[3]);      
  Serial.println(buff);
  
  Serial.println("client connecting to access point...");
  Serial.print("SSID: ");
  Serial.println(nwc.nssid);
  Serial.print("Password: >");
  Serial.print(nwc.npassword);
  Serial.println("<");
  if ( nwc.npassword[0] == 0 ){
    WiFi.begin((char*)nwc.nssid);                         // connect to unencrypted access point      
  }else{
    WiFi.begin((char*)nwc.nssid, (char*)nwc.npassword);   // connect to access point with encryption
  }
  while (( WiFi.status() != WL_CONNECTED ) && ( j < 45 )) {
   j = j + 1 ;
   delay(500);
   Serial.print("+");
   display.clear();
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 0, "Chip ID " + String(ESP.getChipId(), HEX) );
   display.drawString(0, 9, String("SSID:") );
   display.drawString(0, 18, String("Password:") );
   display.setTextAlignment(TEXT_ALIGN_RIGHT);
   display.drawString(128 , 0, String(WiFi.RSSI()));
   display.drawString(128, 9, String(nwc.nssid) );
   display.drawString(128, 18, String(nwc.npassword) );
   display.drawString(j*3, 27 , String(">") );
   display.drawString(0, 36 , String(1.0*j/2) + String(" (s)" ));   
//   snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", nwc.MyIPC[0],nwc.MyIPC[1],nwc.MyIPC[2],nwc.MyIPC[3]);      
   display.drawString(63 , 54 ,  String(buff) );
   display.display();  
  } 
  if ( j >= 45 ) {
   Serial.println("");
   Serial.println("Connection to " + String() + " Failed");
  }else{
    nwc.MyIP = WiFi.localIP() ;
    Serial.println("");
    Serial.print("Connected to " + String(nwc.nssid) + " IP ");
     snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", nwc.MyIP[0],nwc.MyIP[1],nwc.MyIP[2],nwc.MyIP[3]);      
    Serial.print(buff) ;
    nwc.bNWG = true ;
  }

  if ( nwc.NodeName == 0 ){
    sprintf(nwc.NodeName,"Control_%08X\0",ESP.getChipId());  
  }
/*  if (MDNS.begin(nwc.NodeName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
    Serial.print("You can now connect to http://");
    Serial.print(nwc.NodeName);
    Serial.println(".local");
  }
*/  
  server.on("/", handleRoot);
  server.on("/setup", handleSetup);
  server.on("/scan", i2cScan);
  server.on("/stime", handleRoot);  
  server.on("/EEPROM",DisplayEEPROM);
  server.onNotFound(handleNotFound);   
  server.begin();
  
  Serial.println("HTTP server started...");

  pinMode(BUILTIN_LED,OUTPUT);
  pinMode(MOTOR1,OUTPUT);
  pinMode(BEEPER,OUTPUT);
  pinMode(SCOPE_PIN,OUTPUT);
  digitalWrite(MOTOR1,LOW);   // Off I think
  digitalWrite(BEEPER,LOW); 
  
//  EEPROM.begin(512);
//  dnsServer.setTTL(300);
//  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
//  dnsServer.start(53,"injector.local",nwc.myIP);

  rtc.tc.mon = 0 ;
  rtc.tc.wday = 0 ;
  DS3231_init(DS3231_INTCN); // look for a rtc
  DS3231_get(&rtc.tc);
  rtc.rtc_status = DS3231_get_sreg();
  if (((rtc.tc.mon < 1 )|| (rtc.tc.mon > 12 ))&& (rtc.tc.wday>8)){  // no rtc to load off
    Serial.println(F("NO RTC ?"));
  }else{
    setTime((int)rtc.tc.hour,(int)rtc.tc.min,(int)rtc.tc.sec,(int)rtc.tc.mday,(int)rtc.tc.mon,(int)rtc.tc.year ) ; // set the internal RTC
    rtc.bHasRTC = true ;
    Serial.println(F("Has RTC ?"));
    rtc.rtc_temp = DS3231_get_treg(); 
  }
  rtc.rtc_min = minute();
  rtc.rtc_sec = second();

  OTAWebUpdater.setup(&OTAWebServer);
  OTAWebServer.begin(); 
  if ( nwc.bNWG ) {
    if (( nwc.uiThorPort > 0 ) && (nwc.uiThorPort < 30000)){
      ctrludp.begin(nwc.uiThorPort);                 // recieve on the control port  
      Serial.print(F("T"));
    }
    if (( nwc.localPort > 0 ) && (nwc.localPort < 30000)){
      ntpudp.begin(nwc.localPort);                   // this is the recieve on NTP port
      Serial.print(F("N"));
    }
    if (( nwc.uiModbusPort > 0 ) && (nwc.uiModbusPort < 30000)){
      modbusudp.begin(nwc.uiModbusPort);
      Serial.print(F("M"));
    }
    Serial.println(F(" UDP Listerers started...."));
  }
}
// ############################################# LOOP ##########################################################
void loop() {
long lTime ;  
long lRet ;
long lTmpOnCount ;

  server.handleClient();
  OTAWebServer.handleClient();

  lTime = millis() ;                                   // Get the lo res timer
  pcs.lOffCounter = constrain(pcs.lOffCounter,0, pcs.lOffTime);    // anti LA LA land code
  if (pcs.lOnOff != 2 ) {
    if (pcs.lPrime > pcs.lOnTime ){
      pcs.lOnCounter = constrain(pcs.lOnCounter,0, pcs.lPrime);    
    }else{
      pcs.lOnCounter = constrain(pcs.lOnCounter,0, pcs.lOnTime);      
    }
  }
  pcs.lCyclesCounter = constrain(pcs.lCyclesCounter,0, CYCLE_COUNT_MAX ); 

  if ( pcs.lCycles > 0 ) {                                       // if we have cycle or quantity limmits check them
    if (( pcs.lOnOff != 0 ) && ( pcs.lCyclesCounter >= pcs.lCycles )) {  // if out then shut off
      pcs.bState = false ;
      pcs.iFinish = 1 ;
      pcs.lOnOff = 0 ;
    }
  }
  if ( pcs.dblQty > 0 ){                                         // if we have cycle or quantity limmits check them
    if (( pcs.dblCurrentQty >= pcs.dblQty ) && ( pcs.lOnOff != 0 )) {    // if out then shut off
      pcs.bState = false ;
      pcs.iFinish = 1 ;
      pcs.lOnOff = 0 ;
    }
  }
  if (( pcs.lTimePrev2  ) < lTime ){  // look every 2 ms
    if ((pcs.bState)|| (pcs.bPrime )){           // soft starter
      if ( pcs.iOut < 1023 ){  // speed up
        pcs.iOut = pcs.iOut + pcs.PWM_inc ;
        if ( pcs.iOut > 1023 ){
          pcs.iOut = 1023 ;
        }
      }
    }else{
      if ( pcs.iOut > 0  ){   // slow down 
        pcs.iOut = pcs.iOut - pcs.PWM_inc ;
        if ( pcs.iOut < 0 ){
          pcs.iOut = 0 ;
        }
      }
    }
    pcs.lTimePrev2 = lTime + 1 ;
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
  display.drawString(0 , 0, String(buff) );
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(127 , 0, String(WiFi.RSSI()));
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", nwc.MyIPC[0],nwc.MyIPC[1],nwc.MyIPC[2],nwc.MyIPC[3]);    
  if (( nwc.bNWG ) && (( rtc.rtc_sec & 0x02 ) == 0  )){
    snprintf(buff, BUFF_MAX, "IP %03u.%03u.%03u.%03u", nwc.MyIP[0],nwc.MyIP[1],nwc.MyIP[2],nwc.MyIP[3]);          
  }
  display.drawString(63 , 54 ,  String(buff) );

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(127 , 9, String(pcs.lCyclesCounter));
  display.drawString(127 , 18, String(pcs.lOnCounter));
  display.drawString(127 , 27 , String(pcs.lOffCounter));
  display.drawString(127 , 36, String(pcs.dblCurrentQty,2));
  display.drawString(80 , 18, String(pcs.lOnTime));
  display.drawString(80 , 27 , String(pcs.lOffTime));
  display.drawString(80 , 36, String(pcs.dblQty,2));
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  snprintf(buff, BUFF_MAX, "%02d:%02d:%02d", hour(PacketTime), minute(PacketTime), second(PacketTime));
  display.drawString(0 , 45, String(buff) + " >"+ LastPacket + "<");


  display.setTextAlignment(TEXT_ALIGN_LEFT);
  switch (pcs.lOnOff){
    case 1: // ON free run 
      display.drawString(0 , 9, String("Pmp -ON-"));
    break;  
    case 2: // auto
      display.drawString(0 , 9, String("Pmp AUTO"));
    break;  
    default: // off
      display.drawString(0 , 9, String("Pmp OFF"));
    break;
  }

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(80 , 9, String("Cyls"));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0 , 18, String("TOn"));
  display.drawString(0 , 27, String("TOff"));
  display.drawString(0 , 36, String("Qty"));  
  display.display();
    
  if (pcs.lTimePrev > ( lTime + 100000 )){ // has wrapped around so back to zero
    pcs.lTimePrev = lTime ; // skip a bit 
    Serial.println("Wrap around");
  }
  
  if (( pcs.lTimePrev + 100 ) < lTime ){  // look every 1/10 of a second ish
    if (( pcs.bState) || ( pcs.bPrime )) {  // true is on - assume called once per second
      pcs.lOnCounter+= (lTime - pcs.lTimePrev)  ;
      if (pcs.bPrime){
        if ( pcs.lOnCounter >= pcs.lPrime ){
          Serial.println("prime complete");
          pcs.lOnCounter = 0 ;
          pcs.bPrime = false ;
          pcs.bState = false ;
        }      
      }else{
        if ( pcs.lOnOff == 2 ){
          lTmpOnCount = pcs.lWebOnTime ;
        }else{
          lTmpOnCount = pcs.lOnTime ;
        }
        if ( pcs.lOnCounter >= lTmpOnCount ){
          pcs.lOnCounter = 0 ;
          pcs.lCyclesCounter++ ;
          pcs.bState = !pcs.bState ;
        }
      }
      pcs.dblCurrentQty += ( pcs.dblMLPerSecond * (float(lTime - pcs.lTimePrev)/1000))/1000 ;    //work out how much
    }else{
      switch ( pcs.lOnOff ){
        case 1:
          pcs.lOffCounter += (lTime - pcs.lTimePrev) ;
          digitalWrite(BUILTIN_LED,!digitalRead(BUILTIN_LED));
          if ( pcs.lOffCounter >= pcs.lOffTime ) {
            pcs.lOffCounter = 0 ;      
            pcs.bState = !pcs.bState ;
          }
        break;
        default:  // 0 and 2 
          pcs.lOffCounter = 0 ;              
        break;  
      }
    }
    pcs.lTimePrev += 100 ; 
  }
  if ( pcs.PWM_inc > 0 ){
    analogWrite(MOTOR1,pcs.iOut) ; // soft start the motor
  }else{
    if (pcs.bState) {
      digitalWrite(MOTOR1,true) ; // using a relay clunck clunk
    }else{
      digitalWrite(MOTOR1,false) ;     
    }
  }
  
  lScanCtr++ ;
  if (rtc.rtc_sec != second() )  {
    lScanLast = lScanCtr ;
    lScanCtr = 0 ;

    digitalWrite(BUILTIN_LED,!digitalRead(BUILTIN_LED));
    if (pcs.iFinish < 31 ){
      pcs.iFinish++ ;
    }
    if ((pcs.iFinish < 30 )&& (pcs.iFinish >0 )){
      digitalWrite(BEEPER,!digitalRead(BEEPER));
    }else{
      digitalWrite(BEEPER,0);      
    }
    rtc.rtc_sec = second();
  }
  if (rtc.rtc_hour != hour()){  // hourly stuff
    if ( nwc.bNWG ) { // ie we have a network
      sendNTPpacket(nwc.TimeServer); // send an NTP packet to a time server  once and hour
    }else{
      if ( rtc.bHasRTC ){
        DS3231_get(&rtc.tc);
        setTime((int)rtc.tc.hour,(int)rtc.tc.min,(int)rtc.tc.sec,(int)rtc.tc.mday,(int)rtc.tc.mon,(int)rtc.tc.year ) ; // set the internal RTC
      }
    }
    rtc.rtc_hour = hour();
  }
  if ( rtc.rtc_min != minute()){
    if (( year() < 2019 )|| (rtc.bDoTimeUpdate)) {  // not the correct time try to fix every minute
      if ( nwc.bNWG ) { // ie we have a network
        sendNTPpacket(nwc.TimeServer); // send an NTP packet to a time server  
        rtc.bDoTimeUpdate = false ;
      }
    }
    if ( rtc.bHasRTC ){
      rtc.rtc_temp = DS3231_get_treg(); 
    }
    rtc.rtc_min = minute() ;
  }
  
  digitalWrite(SCOPE_PIN,!digitalRead(SCOPE_PIN));  // my scope says we are doing this loop at an unreasonable speed except when we do web stuff

  lRet = ctrludp.parsePacket() ;
  if ( lRet != 0 ) {
    processCtrlUDPpacket(lRet);
  }
  
  lRet = ntpudp.parsePacket() ;
  if ( lRet != 0 ) {
    processNTPpacket();
  }
  
  lRet = modbusudp.parsePacket() ;
  if ( lRet != 0 ) {
    ProcessModbusPacket();
  }
}


unsigned long processCtrlUDPpacket(long lSize){
int i , j ;  
int myslot , mybit , mytime  ;
byte packetBuffer[16] ;                                //  buffer to hold incomming packets  

  memset(packetBuffer, 0, sizeof(packetBuffer));
  ctrludp.read(packetBuffer, sizeof(packetBuffer));    // read the packet into the buffer
  Serial.print(F("Process Ctrl Packet "));
  Serial.println(String((char *)packetBuffer));

  sscanf((char *) packetBuffer , "%d %d %d" , &myslot , &mybit , &mytime );
  if (( myslot == pcs.iMatchSlot ) && ( mybit == pcs.iMatchBit ) && ( mytime > 0  ) && ( mytime < 32000  )){   // is our address or a braodcast
    LastPacket = String((char *)packetBuffer) ;
    PacketTime = now() ;
    pcs.lWebOnTime  = mytime * 100 ; // go from tenths of a second to ms
    if ( pcs.lOnOff != 0 ){  
      pcs.bState = true ;             // enable the on counter
      pcs.lOnCounter = 0 ;            // set it to zero
      pcs.lCyclesCounter++ ;
      pcs.lTimePrev = millis() ;      // zero out the residual
    }
  }

  while (ctrludp.available()){  // clean out the rest of the packet and dump overboard
    ctrludp.read(packetBuffer, sizeof(packetBuffer));  
  }
  return(0);
}

void SerialOutParams(){
String message ;
  message = "Web Request URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  Serial.println(message);
  message = "";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  Serial.println(message);
}




