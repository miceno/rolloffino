#ifndef __rolloff_linear_actuator_parser__
#define __rolloff_linear_actuator_parser__

/*
Command Format:  (command:target|state:value)
Response Format: (response:target|state:value)

Command:
       CON 	(CON:0:0)              Establish initial connection with Arduino
       GET	(GET:state:value)      Get state of a switch
       SET 	(SET:target:value)     Set relay closed or open

           state:   OPENED | CLOSED | LOCKED | AUXSTATE
           target:  OPEN | CLOSE | ABORT | LOCK | AUXSET
           value:   ON | OFF | 0 | text-message

Response:
       ACK       Success returned from Arduino
       NAK       Failure returned from Arduino

Examples:        From the Driver      Response From the Arduino
                 ----------------     --------------------------
Initial connect (CON:0:0)          >
                                   <  (ACK:0:version) | (NAK:ERROR:message)
Read a switch   (GET:OPENED:0)     >
                                   <  (ACK:OPENED:ON|OFF) | (NAK:ERROR:message)
Set a relay     (SET:CLOSE:ON|OFF) >
                                   <  (ACK:CLOSE:ON|OFF) | (NAK:ERROR:message)
*/


void sendAck(char* val) {
  char response[MAX_RESPONSE];
  DEBUG_INFO("ACK=%s", val);  // DEBUG
  if (strlen(val) > MAX_MESSAGE) {
    strncpy(response, val, MAX_MESSAGE - 3);
    strcpy(&response[MAX_MESSAGE - 3], "...");
    sendNak(ERROR1);
    sendNak(response);
  } else {
    strcpy(response, "(ACK:");
    strcat(response, target);
    strcat(response, ":");
    strcat(response, val);
    strcat(response, ")");
    if (USE_WIFI == 1) {
      DEBUG_VERBOSE("about to send response: %s", response);  // DEBUG
      client.println(response);
      client.flush();
    } else {
      Serial.println(response);
      Serial.flush();
    }
  }
}

void sendNak(const char* errorMsg) {
  char buffer[MAX_RESPONSE];
  DEBUG_INFO("NACK=%s", errorMsg);  // DEBUG
  if (strlen(errorMsg) > MAX_MESSAGE) {
    strncpy(buffer, errorMsg, MAX_MESSAGE - 3);
    strcpy(&buffer[MAX_MESSAGE - 3], "...");
    sendNak(ERROR2);
    sendNak(buffer);
  } else {
    strcpy(buffer, "(NAK:ERROR:");
    strcat(buffer, value);
    strcat(buffer, ":");
    strcat(buffer, errorMsg);
    strcat(buffer, ")");
    if (USE_WIFI == 1) {
      client.println(buffer);
      // client.flush();
    } else {
      Serial.println(buffer);
      Serial.flush();
    }
  }
}


int read_data(char* inpBuf, int offset) {
  int recv_count = 0;
  if (USE_WIFI == 1) {
    if (client.available() > 0) {
      recv_count = client.read((unsigned char*)inpBuf + offset, 1);
      DEBUG_DEBUG("Reading data: %d '%s'", recv_count, inpBuf);  // DEBUG
    } else {
      DEBUG_VERBOSE("read data no data available");  // DEBUG
    }
  } else {
    if (Serial.available() > 0) {
      Serial.setTimeout(1000);
      recv_count = Serial.readBytes((inpBuf + offset), 1);
    }
  }
  return recv_count;
}

char* safeStrTok(char* input, const char* delimiter, char* output) {
  char* pointer = NULL;
  pointer = strtok(input, delimiter);
  if (pointer)
    strcpy(output, pointer);
  else
    output[0] = '\0';
  return pointer;
}

bool receiveCommand()  // (command:target:value)
{
  bool start = false;
  bool eof = false;
  int recv_count = 0;
  int wait = 0;
  int offset = 0;
  char startToken = '(';
  char endToken = ')';
  const int bLen = MAX_INPUT;
  char inpBuf[bLen + 1];

  memset(inpBuf, 0, sizeof(inpBuf));
  memset(command, 0, sizeof(command));
  memset(target, 0, sizeof(target));
  memset(value, 0, sizeof(value));

  while (!eof && (wait < 20)) {
    recv_count = read_data(inpBuf, offset);
    if (recv_count == 1) {
      offset++;
      if (offset >= MAX_INPUT) {
        sendNak(ERROR3);
        return false;
      }
      if (inpBuf[offset - 1] == startToken) {
        start = true;
      }
      if (inpBuf[offset - 1] == endToken) {
        eof = true;
        inpBuf[offset] = '\0';
      }
      continue;
    }
    wait++;
    // delay(100);
  }
  DEBUG_INFO("Received command=%s", inpBuf);  // DEBUG

  if (!start || !eof) {
    if (!start && !eof) {
      sendNak(ERROR4);
    } else if (!start) {
      sendNak(ERROR5);
    } else if (!eof) {
      sendNak(ERROR6);
    }
    return false;
  } else {
    safeStrTok(inpBuf, "(:", command);
    safeStrTok(NULL, ":", target);
    safeStrTok(NULL, ")", value);
    if ((strlen(command) >= 3) && (strlen(target) >= 1) && (strlen(value) >= 1)) {
      DEBUG_INFO("cmd=%s, t=%s, v=%s", command, target, value);  // DEBUG
      return true;
    } else {
      sendNak(ERROR7);
      return false;
    }
  }
}


bool is_data_available() {
  bool result = false;

  if (USE_WIFI == 1)
    result = (client && (client.available() > 0));
  else
    result = (Serial && (Serial.available() > 0));

  return result;
}
/*
 * Use the parseCommand routine to decode message
 * Determine associated action in the message. Resolve the relay or switch associated
 * pin with the target identity. Acknowledge any initial connection request. Return
 * negative acknowledgement with message for any errors found.  Dispatch to commandReceived
 * or requestReceived routines to activate the command or get the requested switch state
 */
void parseCommand(Motor *m) {
  // Confirm there is input available, read and parse it.
  if (is_data_available()) {
    DEBUG_VERBOSE("Data is available");  // DEBUG
    if (receiveCommand()) {
      unsigned long timeNow = millis();
      int relay = -1;  // -1 = not found, 0 = not implemented, pin number = supported
      int sw = -1;     //      "                 "                    "
      bool connecting = false;
      const char* error = ERROR8;

      // On initial connection return the version
      if (strcmp(command, "CON") == 0) {
        connecting = true;
        strcpy(value, m->getVersion());  // Can be seen on host to confirm what is running
        m->oledConsole->log(value);
        m->runCommand(CMD_CONNECT, value);
        sendAck(value);
      }

      // Map the general input command term to the local action
      // SET: OPEN, CLOSE, ABORT, LOCK, AUXSET
      else if (strcmp(command, "SET") == 0) {
        // Prepare to OPEN
        if (strcmp(target, "OPEN") == 0) {
          command_input = CMD_OPEN;
          relay = FUNC_ACTIVATION;
        }
        // Prepare to CLOSE
        else if (strcmp(target, "CLOSE") == 0) {
          command_input = CMD_CLOSE;
          relay = FUNC_ACTIVATION;
        }
        // Prepare to ABORT
        else if (strcmp(target, "ABORT") == 0) {
          command_input = CMD_STOP;

          // Test whether or not to Abort
          if (!m->isStopAllowed()) {
            error = ERROR10;
          } else {
            relay = FUNC_STOP;
          }
        }
        // Prepare to ABORT
        else if (strcmp(target, "STOP") == 0) {
          command_input = CMD_STOP;
          relay = FUNC_STOP;
        }
        // Prepare to ABORT
        else if (strcmp(target, "DISABLE") == 0) {
          command_input = CMD_STOP;
          relay = FUNC_STOP;
        }
        // Prepare for the Lock function
        else if (strcmp(target, "LOCK") == 0) {
          command_input = CMD_LOCK;
          relay = FUNC_LOCK;
        }

        // Prepare for the Auxiliary function
        else if (strcmp(target, "AUXSET") == 0) {
          command_input = CMD_AUXSET;
          relay = FUNC_AUX;
        }
      }

      // Handle requests to obtain the status of switches
      // GET: OPENED, CLOSED, LOCKED, AUXSTATE
      else if (strcmp(command, "GET") == 0) {
        if (strcmp(target, "OPENED") == 0) {
          sw = SWITCH_OPENED;
        } else if (strcmp(target, "CLOSED") == 0) {
          sw = SWITCH_CLOSED;
        } else if (strcmp(target, "LOCKED") == 0) {
          sw = SWITCH_LOCKED;
        } else if (strcmp(target, "AUXSTATE") == 0) {
          sw = SWITCH_AUX;
        }
      }

      /*
       * See if there was a valid command or request
       */
      if (!connecting) {
        if ((relay == -1) && (sw == -1)) {
          sendNak(error);  // Unknown input or Abort command was rejected
        }

        // Command or Request not implemented
        else if ((relay == 0 || relay == -1) && (sw == 0 || sw == -1)) {
          DEBUG_ERROR("Command or request not implemented");  // DEBUG
          strcpy(value, "OFF");                               // Request Not implemented
          //sendNak(ERROR9);
          sendAck(value);
        }

        // Valid input received

        // A command was received
        // Set the relay associated with the command and send acknowlege to host
        else if (relay > 0)  // Set Relay response
        {
          m->runCommand(command_input, value);
          // Send acknowledgement that relay pin associated with "target" was activated to value requested
          sendAck(value);
        }

        // A state request was received
        else if (sw > 0)  // Get switch response
        {
          DEBUG_VERBOSE("about to get Status");  // DEBUG
          getSwitch(sw, value);
          sendAck(value);  // Send result of reading pin associated with "target"
        }
      }  // end !connecting
    }    // end command parsed
  }      // end input found
  else {
    DEBUG_DEBUG("No data available. Continue...");  // DEBUG
  }
}


#endif