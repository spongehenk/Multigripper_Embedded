const long BAUDRATE = 250000; // Computer <-> Arduino serial Baudrate.
const int DELAY_TIME = 100; // ms
enum SYS_STATUS { STOPPED, PAUSED, RUNNING, EMERGENCY };
int systemStatus = STOPPED;

void setup() {
  Serial.begin(BAUDRATE);

  /* Zigbee initialisation code should come here
   *  
   */
}

void loop() {
  checkSerial();
}

/* 
 *  This function checks if data is sent from the computer over 
 *  the serial port. If so, it checks the validity of the command
 *  and acts accordingly.
 */
 
void checkSerial() {

  // Check if new data is available
  if (Serial.available()) 
  {
    String data = Serial.readString(); // Read \r\n ending serial string
    int dataLength = data.length() - 1; // Somehow, arduino counts 1 extra when using the standard Serial Monitor

    // Error marking boolean; used throughout the serial data processing code:
    boolean error = false;
    
    if (dataLength > 1)  // Check if the string has at least two characters:
    {
      
      // Extract the first two bits from the sent string:
      String identifier = data.substring(0,2);
      
      // Act according to command sent:
      // ------------------------------------------------------------

      if (identifier == "Rp") // Receive positions command
      {
        String gripperIDText = data.substring(3, dataLength); // take IdGripper value
        long gripperID = gripperIDText.toInt(); // type conversion
        // (will be 0 if unable to convert)
        
        receivePositionsCommand(&gripperID); // in commands.ino
      }

      else if (identifier == "Rc") // Receive controller info command
      {
        String gripperIDText = data.substring(3, dataLength); // take IdGripper value
        long gripperID = gripperIDText.toInt(); // type conversion
        // (will be 0 if unable to convert)
        
        receiveControllerInfoCommand(&gripperID); // in commands.ino
      }
      
      else if (identifier == "Sp") // Send position/setpoint command
      {
        int semicolonIndex = findNextSemicolon(data, 3); // find next semicolon char index

        if (semicolonIndex < 0) // function returns -1 in case no semicolon is found
        {
          error = true; // if this is the case, throw an error
        }
        else 
        {
          String gripperIDText = data.substring(3, semicolonIndex + 1); // take IdGripper value
          long gripperID = gripperIDText.toInt(); // type conversion
          // (will be 0 if unable to convert)
          
          int dataEnd = findNextSemicolon(data, semicolonIndex+1);
          if (dataEnd < 0) // function returns -1 in case no semicolon is found
          {
            error = true; // if this is the case, throw an error
          }
          else 
          {
            String setpointText = data.substring(semicolonIndex, dataEnd-1);
            long setpoint = setpointText.toInt();
            semicolonIndex = findNextSemicolon(data, dataEnd+1); // find next semicolon char index
            
            if (semicolonIndex < 0) // function returns -1 in case no semicolon is found
            {
              error = true; // if this is the case, throw an error
            }
            else 
            {
              String accText = data.substring(dataEnd, semicolonIndex-1);
              long acc = accText.toInt();
              
              String velText = data.substring(semicolonIndex, dataLength);
              long vel = velText.toInt();
              
              sendPositionCommand(&gripperID, &setpoint, &acc, &vel); // in commands.ino
              
            }
          }
        }
      }

      else if (identifier == "Rs") // Get status function
      {
        receiveStatusCommand(); // in commands.ino
      }
      
      else if (identifier == "Ss") // Send status function
      {
        String statusText = data.substring(3, dataLength); // The rest of the data is the status
        long statusInteger = statusText.toInt(); // type conversion
        // (will be 0 if unable to convert), which corresponds to "stopped"

        sendStatusCommand(&statusInteger);  // in commands.ino 
      }
      
      else if (identifier == "Sc") // Send controller settings function
      {
        int dataEnd = findNextSemicolon(data, 3); // find next semicolon
        
        if (dataEnd < 0) // function returns -1 in case no semicolon is found
        {
          error = true; // if this is the case, throw an error
        }
        else 
        {
          String gripperIDText = data.substring(3, dataEnd); // take IdGripper value
          long gripperID = gripperIDText.toInt(); // type conversion
          // (will be 0 if unable to convert)
  
          int semicolonIndex = findNextSemicolon(data, dataEnd+1); // find next semicolon 
          
          if (semicolonIndex < 0) // function returns -1 in case no semicolon is found
          {
            error = true; // if this is the case, throw an error
          }
          else
          {
            String KpText = data.substring(dataEnd, semicolonIndex-1); // Extract "Kp";
            long Kp = KpText.toInt(); // type conversion
            // (will be 0 if unable to convert)
    
            // dataEnd value is re-used here to save precious RAM
            dataEnd = findNextSemicolon(data, semicolonIndex+1); // find next semicolon 
            if (dataEnd < 0) // function returns -1 in case no semicolon is found
            {
              error = true; // if this is the case, throw an error
            }
            else 
            {
              String KiText = data.substring(semicolonIndex, dataEnd-1); // Extract "Ki";
              long Ki = KiText.toInt(); // type conversion
              // (will be 0 if unable to convert)
              
              // semicolonIndex value is re-used too
              semicolonIndex = findNextSemicolon(data, dataEnd+1); // find next semicolon 
              if (semicolonIndex < 0) // function returns -1 in case no semicolon is found
              {
                error = true; // if this is the case, throw an error
              }
              else
              {
                String KdText = data.substring(dataEnd, semicolonIndex-1); // Extract "Ki";
                long Kd = KdText.toInt(); // type conversion
                // (will be 0 if unable to convert)
                
                String LPFText = data.substring(semicolonIndex, dataLength);
                long LPF = LPFText.toInt(); // type conversion
                // (will be 0 if unable to convert)
                sendControllerSettingsCommand(&gripperID, &Kp, &Ki, &Kd, &LPF); // in commands.ino
               
              }
            }
          }
        }
      }
      
    } 
    else 
    { // String received has less than two characters, print error:
      error = true;
    }

    // This makes it possible to enhance the "data checks" later on.
    // For example, if the [x]th semicolon couldn't be found, but it is 
    // not expected, you can easily set [error] to true
    // This is not implemented yet nevertheless
    if (error == true) 
    {
      Serial.println("INVALID");
    }
  }
  
}

/*
 * This function loops the characters of the given string [data] 
 * from index [start] onwards, and tries to find the next 
 * semicolon. It returns the index of this semicolon
 */
int findNextSemicolon (String data, int start)
{
  // now loop through all characters until the next semicolon
  boolean semicolonReached = false;
  
  while (semicolonReached == false && start < data.length())
  {
    if (data.charAt(start) == ';') {
      semicolonReached = true;
    }
    start++;
  }

  // return -1 when no semicolon is found
  if (semicolonReached == false) 
  {
    return -1;
  }
  else // return found semicolon index if found
  {
    return start;
  }
}
