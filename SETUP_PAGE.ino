void handleSetup() {
int i ;
String MyColor ;
byte mac[6];
//  SerialOutParams();
  for (uint8_t j=0; j<server.args(); j++){
    i = String(server.argName(j)).indexOf("ndadd");
    if (i != -1){  // 
      nwc.lNodeAddress = String(server.arg(j)).toInt() ;
      constrain(nwc.lNodeAddress,0,32768);
    }        
    i = String(server.argName(j)).indexOf("tzone");
    if (i != -1){  // 
      nwc.fTimeZone = String(server.arg(j)).toFloat() ;
      constrain(nwc.fTimeZone,-12,12);
      rtc.bDoTimeUpdate = true ; // trigger and update to fix the time
    }        
    i = String(server.argName(j)).indexOf("disop");
    if (i != -1){  // 
      lDisplayOptions = String(server.arg(j)).toInt() ;
      constrain(lDisplayOptions,0,255);
    }  
    i = String(server.argName(j)).indexOf("lpntp");
    if (i != -1){  // 
      nwc.localPort = String(server.arg(j)).toInt() ;
      constrain(nwc.localPort,1,65535);
    }        
    i = String(server.argName(j)).indexOf("lpctr");
    if (i != -1){  // 
      nwc.localPortCtrl = String(server.arg(j)).toInt() ;
      constrain(nwc.localPortCtrl,1,65535);
    }        
    i = String(server.argName(j)).indexOf("rpctr");
    if (i != -1){  // 
      nwc.localPortCtrl = String(server.arg(j)).toInt() ;
      constrain(nwc.localPortCtrl,1,65535);
    }        
    i = String(server.argName(j)).indexOf("dontp");
    if (i != -1){  // have a request to request a time update
      rtc.bDoTimeUpdate = true ;
    }
    i = String(server.argName(j)).indexOf("cname");
    if (i != -1){  // have a request to request a time update
     String(server.arg(j)).toCharArray( nwc.NodeName , sizeof(nwc.NodeName)) ;
    }
    i = String(server.argName(j)).indexOf("atoff");    
    if (i != -1){  // have a request to request a time update
      rtc.tm.Year = (String(server.arg(j)).substring(0,4).toInt()-1970) ;
      rtc.tm.Month =(String(server.arg(j)).substring(5,7).toInt()) ;
      rtc.tm.Day = (String(server.arg(j)).substring(8,10).toInt()) ;
      rtc.tm.Hour =(String(server.arg(j)).substring(11,13).toInt()) ;
      rtc.tm.Minute = (String(server.arg(j)).substring(14,16).toInt()) ;
      rtc.tm.Second = 0 ;
      pcs.AutoOff_t = makeTime(rtc.tm);
    }  
    i = String(server.argName(j)).indexOf("nssid");
    if (i != -1){                                    // SSID
 //    Serial.println("SookyLala 1 ") ;
     String(server.arg(j)).toCharArray( nwc.nssid , sizeof(nwc.nssid)) ;
    }
    i = String(server.argName(j)).indexOf("npass");
    if (i != -1){                                    // Password
     String(server.arg(j)).toCharArray( nwc.npassword , sizeof(nwc.npassword)) ;
    }    
    i = String(server.argName(j)).indexOf("sssid");
    if (i != -1){                                    // SSID
 //    Serial.println("SookyLala 1 ") ;
     String(server.arg(j)).toCharArray( nwc.cssid , sizeof(nwc.nssid)) ;
    }
    i = String(server.argName(j)).indexOf("spass");
    if (i != -1){                                    // Password
     String(server.arg(j)).toCharArray( nwc.cpassword , sizeof(nwc.npassword)) ;
    }    
  }
  
  SendHTTPHeader();

  server.sendContent("<form method=get action=" + server.uri() + "><table border=1 title='Node Settings'>");
  server.sendContent(F("<tr><th width=100>Parameter</th><th width=165>Value</th><th width=55><input type='submit' value='SET'></th></tr>"));

  server.sendContent(F("<tr><td>Controler Name</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='cname' value='"+String(nwc.NodeName)+"' maxlength=15 size=12></td><td></td></tr>");

  snprintf(buff, BUFF_MAX, "%04d/%02d/%02d %02d:%02d", year(pcs.AutoOff_t), month(pcs.AutoOff_t), day(pcs.AutoOff_t) , hour(pcs.AutoOff_t), minute(pcs.AutoOff_t));
  if (pcs.AutoOff_t > now()){
    MyColor =  F("bgcolor=red") ;
  }else{
    MyColor =  "" ;
  }
  server.sendContent("<tr><td "+String(MyColor)+">Auto Off Until</td><td align=center>") ; 
  server.sendContent("<input type='text' name='atoff' value='"+ String(buff) + "' size=12></td><td>(yyyy/mm/dd)</td></tr>");

  server.sendContent(F("<tr><td>Time Zone</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='tzone' value='" + String(nwc.fTimeZone,1) + "' size=12></td><td>(Hours)</td></tr>");

  server.sendContent(F("<tr><td>Display Options</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='disop' value='" + String(lDisplayOptions) + "' size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Local UDP Port NTP</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='lpntp' value='" + String(nwc.localPort) + "' size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Local UDP Port Control</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='lpctr' value='" + String(nwc.localPortCtrl) + "' size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Remote UDP Port Control</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='rpctr' value='" + String(nwc.RemotePortCtrl) + "' size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Network SSID</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='nssid' value='" + String(nwc.nssid) + "' maxlength=31 size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Network Password</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='npass' value='" + String(nwc.npassword) + "' maxlength=15 size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Setup SSID</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='sssid' value='" + String(nwc.cssid) + "' maxlength=31 size=12></td><td></td></tr>");

  server.sendContent(F("<tr><td>Setup Password</td><td align=center>")) ; 
  server.sendContent("<input type='text' name='spass' value='" + String(nwc.cpassword) + "' maxlength=15 size=12></td><td></td></tr>");

  server.sendContent("<tr><td>ESP ID</td><td align=center>0x" + String(ESP.getChipId(), HEX) + "</td><td align=center>"+String(ESP.getChipId())+"</td></tr>" ) ; 
  WiFi.macAddress(mac);      
  snprintf(buff, BUFF_MAX, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  server.sendContent("<tr><td>Station MAC Address</td><td align=center>" + String(buff) + "</td><td align=center>.</td></tr>" ) ; 
  WiFi.softAPmacAddress(mac);
  snprintf(buff, BUFF_MAX, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  server.sendContent("<tr><td>SoftAP MAC Address</td><td align=center>" + String(buff) + "</td><td align=center>.</td></tr>" ) ; 
  snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", nwc.MyIP[0],nwc.MyIP[1],nwc.MyIP[2],nwc.MyIP[3]);
  server.sendContent("<tr><td>Network IP Address</td><td align=center>" + String(buff) + "</td><td>.</td></tr>" ) ; 
  snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", nwc.MyIPC[0],nwc.MyIPC[1],nwc.MyIPC[2],nwc.MyIPC[3]);
  server.sendContent("<tr><td>Soft AP IP Address</td><td align=center>" + String(buff) + "</td><td>.</td></tr>" ) ; 

  server.sendContent("<tr><td>WiFi RSSI</td><td align=center>" + String(WiFi.RSSI()) + "</td><td>(dBm)</td></tr>" ) ; 
  snprintf(buff, BUFF_MAX, "%03u.%03u.%03u.%03u", nwc.MyIP[0],nwc.MyIP[1],nwc.MyIP[2],nwc.MyIP[3]);
  server.sendContent("<tr><td>Node IP Address</td><td align=center>" + String(buff) + "</td><td>.</td></tr>" ) ; 
  server.sendContent("<tr><td>Last Scan Speed</td><td align=center>" + String(lScanLast) + "</td><td>(per second)</td></tr>" ) ;    
  if( rtc.bHasRTC ){
    rtc.rtc_status = DS3231_get_sreg();
    if (( rtc.rtc_status & 0x80 ) != 0 ){
      server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='red'>DEPLETED</td><td></td></tr>")) ;            
    }else{
      server.sendContent(F("<tr><td>RTC Battery</td><td align=center bgcolor='green'>-- OK --</td><td></td></tr>")) ;                    
    }
    server.sendContent("<tr><td>RTC Temperature</td><td align=center>"+String(rtc.rtc_temp,1)+"</td><td>(C)</td></tr>") ;                    
  }
  server.sendContent("<tr><td>ESP Core Version</td><td align=center>" + String(ESP.getCoreVersion()) + "</td><td>.</td></tr>" ) ;    
  server.sendContent("<tr><td>ESP Full Version</td><td align=center>" + String(ESP.getFullVersion()) + "</td><td>.</td></tr>" ) ;    
  server.sendContent("<tr><td>SDK Version</td><td align=center>" + String(ESP.getSdkVersion()) + "</td><td>.</td></tr>" ) ;    
  server.sendContent("<tr><td>CPU Volts</td><td align=center>" + String(ESP.getVcc()) + "</td><td>(V)</td></tr>" ) ;    
  server.sendContent("<tr><td>CPU Frequecy</td><td align=center>" + String(ESP.getCpuFreqMHz()) + "</td><td>(MHz)</td></tr>" ) ;    
  server.sendContent("<tr><td>Get Rest Reason</td><td align=center>" + String(ESP.getResetReason()) + "</td><td></td></tr>" ) ;    
  server.sendContent("<tr><td>Get Reset Into</td><td align=center>" + String(ESP.getResetInfo()) + "</td><td></td></tr>" ) ;    
  server.sendContent("<tr><td>Get Sketch Size</td><td align=center>" + String(ESP.getSketchSize()) + "</td><td>(Bytes)</td></tr>" ) ;    
  server.sendContent("<tr><td>Free Sketch Space</td><td align=center>" + String(ESP.getFreeSketchSpace()) + "</td><td>(Bytes)</td></tr>" ) ;    
  
  server.sendContent(F("</form></table>"));
  SendHTTPPageFooter() ;

}


