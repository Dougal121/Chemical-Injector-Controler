unsigned long SendModbusPacketReply(char* address){
byte packetBuffer[ NTP_PACKET_SIZE];     //buffer to hold incoming and outgoing packets  
  uint8_t * data ; 
  uint8_t bc = 0 ;
  int i ;

  data = (uint8_t *) &pcs ;
  memset(packetBuffer, 0, NTP_PACKET_SIZE);    // set all bytes in the buffer to 0
  packetBuffer[0] = MB.net_id ; 
  packetBuffer[1] = MB.fc ;
  switch(MB.fc){
    case 3:
      bc = MB.wordcount * 2 ;
      packetBuffer[2] = bc ; 
      if ( bc > 32 ){
        packetBuffer[2] = 0 ; 
        packetBuffer[1] |= 0x80 ;   
      }else{
        for ( i = 0 ; i < bc ; i++ ){
          packetBuffer[3+i] = data[i] ;
        }
      }
    break;
    case 16:
    break;
    default:
        bc = 0 ;
        packetBuffer[2] = 0 ; 
        packetBuffer[1] |= 0x80 ;   
    break;
  }

  modbusudp.beginPacket(MB.remotePort, MB.remoteIP );          // send back to the remote port
  modbusudp.write(packetBuffer, (bc+3) );
  modbusudp.endPacket();
}

unsigned long ProcessModbusPacket(void){
  MB.remotePort = modbusudp.remotePort();
  MB.remoteIP = modbusudp.remoteIP();
  modbusudp.read(packetBuffer, NTP_PACKET_SIZE);       // the timestamp starts at byte 40 of the received packet and is four bytes, or two words, long. First, esxtract the two words:
  if ( packetBuffer[6] == 6 ){
    MB.net_id = packetBuffer[7] ;       // usefull stuff starts here
    MB.fc = packetBuffer[8] ;   
    switch(MB.fc){
      case 4:
        MB.address = packetBuffer[8]  << 8 | packetBuffer[9] ;   // get the address 
        MB.wordcount = packetBuffer[10] << 8 | packetBuffer[11] ;  // get the word count 
      break;
      case 16:
      break;
      default:
      break;
    }
    
  }
  while (modbusudp.available()){  // clean out the rest of the packet and dump overboard
    modbusudp.read(packetBuffer, sizeof(packetBuffer));  
  }
}

