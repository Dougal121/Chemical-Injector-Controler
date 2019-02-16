void SendHTTPHeader(){ 
  server.sendHeader("Server","ESP8266-on-steroids",false);
  server.sendHeader("X-Powered-by","Dougal-1.0",false);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent("<!DOCTYPE HTML>");
  server.sendContent("<head><title>Team Trouble - Chemical Injector</title>");
  server.sendContent("<meta name=viewport content='width=320, auto inital-scale=1'>");
  server.sendContent("</head><body><html><center><h2>Chemical Injector " + String(Toleo) + "</h2>");
  server.sendContent("<a href='/'>Refresh</a><br><br>") ;         
  server.sendContent("<a href='/?command=2'>Save Parameters to EEPROM</a><br>") ;         
}

void SendHTTPPageFooter(){
  server.sendContent(F("<br><a href='/?command=1'>Load Parameters from EEPROM</a><br><br><br><a href='/?command=667'>Reset Memory to Factory Default</a><br><a href='/?command=665'>Sync UTP Time</a><br><a href='/stime'>Manual Time Set</a><br><a href='/scan'>I2C Scan</a><br><a href='/?command=9'>Reboot</a><br>"));   
  server.sendContent(F("<a href='/setup'>Node/Network Setup</a><br>"));  
  snprintf(buff, BUFF_MAX, "%u.%u.%u.%u", nwc.MyIP[0],nwc.MyIP[1],nwc.MyIP[2],nwc.MyIP[3]);
  server.sendContent("<br><a href='http://" + String(buff) + ":81/update'>OTA Firmware Update</a><br>");
  server.sendContent(F("</body></html>\r\n"));
}

void handleNotFound(){
  String message = "Seriously - No way DUDE\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.print(message);
}

 
void handleRoot() {
  boolean currentLineIsBlank = true;
  char buff[40] ;
  String MyColor ;
  String MyText ;
  long  i = 0 ;
  bool bDefault = true ;

//  SerialOutParams();
  
  for (uint8_t j=0; j<server.args(); j++){
    i = String(server.argName(j)).indexOf("command");
    if (i != -1){  // have a request to set the time zone
      switch (String(server.arg(j)).toInt()){
        case 1:  // load values
          LoadParamsFromEEPROM(true);
          Serial.println("Load from EEPROM");
        break;
        case 2: // Save values
          LoadParamsFromEEPROM(false);
          Serial.println("Save to EEPROM");
        break;
        case 3: // prime command
          pcs.bPrime = true ;
          pcs.bState = true ;          
          Serial.println("Prime Pumps");
        break;
        case 4: // Clear current qty
          pcs.dblCurrentQty = 0 ;
          Serial.println("Clear current qty");
        break;
        case 5: // Clear current qty
          pcs.lCyclesCounter = 0 ;
          Serial.println("Clear current cycles");
        break;
        case 8: //  Cold Reboot
          ESP.reset() ;
        break;
        case 9: //  Warm Reboot
          ESP.restart() ;
        break;        
        case 665:
          sendNTPpacket(nwc.TimeServer); // send an NTP packet to a time server  once and hour  
        break;        
        case 666:
          pcs.lOnOff = 1 ; 
          pcs.bState = true ;          
        break;
        case 667:
          pcs.lOnOff = 2 ; 
          pcs.bState = false ;          
        break;
        case 999:
          pcs.lOnOff = 0 ; 
          pcs.bState = false ;          
        break;
        case 42:
          ESP.reset() ;
        break;
      }  
    }
    i = String(server.argName(j)).indexOf("disop");
    if (i != -1){  // 
      lDisplayOptions = String(server.arg(j)).toInt() ;
      constrain(lDisplayOptions,0,255);
    }  
    
    i = String(server.argName(j)).indexOf("prime");
    if (i != -1){  // have a request to set the time zone
      pcs.lPrime = String(server.arg(j)).toInt() ;
      constrain(pcs.lPrime,500,32000);
    }        

    i = String(server.argName(j)).indexOf("timon");
    if (i != -1){  // have a request to set the time zone
      pcs.lOnTime = String(server.arg(j)).toInt() ;
      constrain(pcs.lOnTime,TIMER_MIN,TIMER_MAX);
    }        
    i = String(server.argName(j)).indexOf("timof");
    if (i != -1){  // have a request to set the time zone
      pcs.lOffTime = String(server.arg(j)).toInt() ;
      constrain(pcs.lOffTime,TIMER_MIN,TIMER_MAX);
    }        
    i = String(server.argName(j)).indexOf("cycnt");
    if (i != -1){  // have a request to set the time zone
      pcs.lCycles = String(server.arg(j)).toInt() ;
      constrain(pcs.lCycles,0,CYCLE_COUNT_MAX);
    }
            
    i = String(server.argName(j)).indexOf("mybit");
    if (i != -1){  // have a request to set the time zone
      pcs.iMatchBit = String(server.arg(j)).toInt() ;
      constrain(pcs.iMatchBit,0,63);
    }        
    i = String(server.argName(j)).indexOf("myslt");
    if (i != -1){  // have a request to set the time zone
      pcs.iMatchSlot = String(server.arg(j)).toInt() ;
      constrain(pcs.iMatchSlot,0,31);
    }        
    
    i = String(server.argName(j)).indexOf("pwminc");
    if (i != -1){  // have a request to set the time zone
      pcs.PWM_inc = String(server.arg(j)).toInt() ;
      constrain(pcs.PWM_inc,PWM_INC_MIN,PWM_INC_MAX);
    }        
    
    i = String(server.argName(j)).indexOf("pumpea");
    if (i != -1){  // have a request to set the time zone
      pcs.lOnOff =  String(server.arg(j)).toInt() ;
      if ( pcs.lOnOff == 1 ) {
          pcs.bState = true ;
      }else{
          pcs.bState = false ;
      }
    }        

    i = String(server.argName(j)).indexOf("pmlps");    // pump ml per second
    if (i != -1){  // have a request to set the latitude
      pcs.dblMLPerSecond = String(server.arg(j)).toFloat() ;
      constrain(pcs.dblMLPerSecond,PUMP_ML_PER_SEC_MIN,PUMP_ML_PER_SEC_MIN);
    }        
    i = String(server.argName(j)).indexOf("quant");    // maximum qty to be pumped
    if (i != -1){  // have a request to set the latitude
      pcs.dblQty = String(server.arg(j)).toFloat() ;
      constrain(pcs.dblQty,0,PUMPED_QTY_MAX);
    }        
    i = String(server.argName(j)).indexOf("stime");
    if (i != -1){  // 
      rtc.tm.Year = (String(server.arg(j)).substring(0,4).toInt()-1970) ;
      rtc.tm.Month =(String(server.arg(j)).substring(5,7).toInt()) ;
      rtc.tm.Day = (String(server.arg(j)).substring(8,10).toInt()) ;
      rtc.tm.Hour =(String(server.arg(j)).substring(11,13).toInt()) ;
      rtc.tm.Minute = (String(server.arg(j)).substring(14,16).toInt()) ;
      rtc.tm.Second = 0 ;
      setTime(makeTime(rtc.tm));    
      if ( rtc.bHasRTC ){
        rtc.tc.sec = second();     
        rtc.tc.min = minute();     
        rtc.tc.hour = hour();   
        rtc.tc.wday = dayOfWeek(makeTime(rtc.tm));            
        rtc.tc.mday = day();  
        rtc.tc.mon = month();   
        rtc.tc.year = year();       
        DS3231_set(rtc.tc);                       // set the RTC as well
        rtc.rtc_status = DS3231_get_sreg();       // get the status
        DS3231_set_sreg(rtc.rtc_status & 0x7f ) ; // clear the clock fail bit when you set the time
      }
    }        
    
  }

  SendHTTPHeader();   //  ################### START OF THE RESPONSE  ######
  if (String(server.uri()).indexOf("stime")>0) {  // ################   SETUP TIME    #######################################
    bDefault = false ;
    snprintf(buff, BUFF_MAX, "%04d/%02d/%02d %02d:%02d", year(), month(), day() , hour(), minute());
    server.sendContent("<br><br><form method=get action=" + server.uri() + "><br>Set Current Time: <input type='text' name='stime' value='"+ String(buff) + "' size=12>");
    server.sendContent(F("<input type='submit' value='SET'><br><br></form>"));
  }
  if (bDefault) {     // #####################################   Default Control ##############################################  
    snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
    if (nwc.fTimeZone > 0 ) {
      server.sendContent("<b>"+ String(buff) + " UTC +" + String(nwc.fTimeZone,1) ) ;   
    }else{
      server.sendContent("<b>"+ String(buff) + " UTC " + String(nwc.fTimeZone,1) ) ;       
    }
    if ( year() < 2000 ) {
      server.sendContent(F("  --- CLOCK NOT SET ---")) ;
    }
    server.sendContent(F("</b><br>")) ;  
    if ( pcs.AutoOff_t > now() )  {
      snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(pcs.AutoOff_t), month(pcs.AutoOff_t), day(pcs.AutoOff_t) , hour(pcs.AutoOff_t), minute(pcs.AutoOff_t), second(pcs.AutoOff_t));
      server.sendContent(F("<b><font color=red>Automation OFFLINE Untill ")) ;  
      server.sendContent(String(buff)) ; 
      server.sendContent(F("</font></b><br>")) ; 
    }else{
      if ( year() > 2000 ) {
        server.sendContent(F("<b><font color=green>Automation ONLINE</font></b><br>")) ;  
      }else{
        server.sendContent(F("<b><font color=green>Automation OFFLINE Invalid time</font></b><br>")) ;        
      }
    }
  
    server.sendContent("<table border=1 title='Pump Control'>");
    server.sendContent("<tr><th> Parameter</th><th>Value</th><th>.</th></tr>");
  
    switch(pcs.lOnOff){
      case 1:
        MyText = String("ON") ;
        MyColor = " BGCOLOR='green' " ;
      break;
      case 2:
        MyText = String("AUTO") ;
        MyColor = " BGCOLOR='yellow' " ;
      break;
      default:
        MyText = String("OFF") ;
        MyColor = " BGCOLOR='red' " ;
      break;
    }
  
    server.sendContent("<tr><td "+ MyColor + ">Pump Control " + MyText + "</td><td align=right><form method=get action=/><input type='hidden' name='pumpea' value='1'><input type='submit' value='ON'></form>") ;
    server.sendContent("<form method=get action=/><input type='hidden' name='pumpea' value='2'><input type='submit' value='AUTO'></form>") ;
    server.sendContent("</td><td><form method=get action=/><input type='hidden' name='pumpea' value='0'><input type='submit' value='OFF'></td></tr></form>");
  
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>Time On (ms)</td><td align=center>") ; 
    server.sendContent("<input type='text' name='timon' value='" + String(pcs.lOnTime) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>Time Off (ms)</td><td align=center>") ; 
    server.sendContent("<input type='text' name='timof' value='" + String(pcs.lOffTime) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td><a href='/?command=3' title='Click to Prime Pump'>Prime Time (ms)</a></td><td align=center>") ; 
    server.sendContent("<input type='text' name='prime' value='" + String(pcs.lPrime) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>Max Cycles</td><td align=center>") ; 
    server.sendContent("<input type='text' name='cycnt' value='" + String(pcs.lCycles) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>ml Per Second</td><td align=center>") ; 
    server.sendContent("<input type='text' name='pmlps' value='" + String(pcs.dblMLPerSecond,2) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>Max Quanity (l)</td><td align=center>") ; 
    server.sendContent("<input type='text' name='quant' value='" + String(pcs.dblQty,2) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "><tr><td>PWM inc (0 Disable)</td><td align=center>") ; 
    server.sendContent("<input type='text' name='pwminc' value='" + String(pcs.PWM_inc) + "' size=6></td><td><input type='submit' value='SET'></td></tr></form>");
  
    server.sendContent("<form method=get action=" + server.uri() + "<tr><td>Display Options</td><td align=center>") ; 
    server.sendContent("<input type='text' name='disop' value='" + String(lDisplayOptions) + "' size=6 maxlength=3></td><td><input type='submit' value='SET'></td></tr></form>");
  
    if( rtc.bHasRTC ){
      rtc.rtc_status = DS3231_get_sreg();
      if (( rtc.rtc_status & 0x80 ) != 0 ){
        server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='red'>DEPLETED</td><td></td></tr>")) ;            
      }else{
        server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='green'>-- OK --</td><td></td></tr>")) ;                    
      }
      server.sendContent("<tr><td>RTC Temperature</td><td align=center>"+String(rtc.rtc_temp,1)+"</td><td>(C)</td></tr>") ;                    
    }
    
    server.sendContent("<tr><td><a href='/?command=4' title='click to clear'>Current Qty</a></td><td align=center>" + String(pcs.dblCurrentQty,3) + "</td><td>(l)</td></tr>");
  //  server.sendContent("<tr><td>Current On Counter</td><td align=center>" + String(pcs.lOnCounter) + "</td><td>(ms)</td></tr>");
  //  server.sendContent("<tr><td>Current Off Counter</td><td align=center>" + String(pcs.lOffCounter) + "</td><td>(ms)</td></tr>");
    server.sendContent("<tr><td><a href='/?command=5' title='click to clear'>Current Cycles</td><td align=center>" + String(pcs.lCyclesCounter) + "</a></td><td>.</td></tr>");
    server.sendContent("</table>");
  }
  SendHTTPPageFooter();
}

