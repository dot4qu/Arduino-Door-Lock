#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>
#include <Keypad.h>

#define UNLOCK_POSITION 25
#define LOCK_POSITION 105

#define BUTTON_PIN 2

//Keypad globals/constants ------------------------------
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {6, 5, 4, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {10, 9, 8, 7}; //connect to the column pinouts of the keypad
//byte keyBuf[4] = {};
//const byte secretCode[4] = {'1', '2', '3', '4'};
String keyBuf = "";
const String secretCode = "8715";
int keyBufIndex = 0;
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//Servo globals ------------------------------
Servo servo;
int prevButtonRead = 0;
int buttonRead = 0;

//Ethernet Globals ------------------------------
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10,0,1,11);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

const String username = "testuser";
const String password = "testpass";
String recievedUsername;
String recievedPassword;
boolean authenticated;
boolean toggling;
boolean lockedStatus;

void parse(String parseString);
void authenticate();

/****************************************************************************************************/

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  servo.attach(A0);

  pinMode(BUTTON_PIN, INPUT);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  recievedUsername = "";
  recievedPassword = "";
  authenticated = false;
  toggling = false;
  lockedStatus = false;

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  servo.write(UNLOCK_POSITION);
}


void loop() {
  //handling buttonpress from inside
  buttonRead = digitalRead(BUTTON_PIN);
  if (!buttonRead && prevButtonRead == 1) {
    toggleLock();
  }
  
  //get keypad button if it's been pressed
  char customKey = customKeypad.getKey();
  //building and checking keybuffer
   if (customKey){
    if (customKey == '*') {
      keyBufIndex = 0;
      keyBuf = "";
      Serial.println("Resetting code");
    } else if (keyBufIndex < 3) {
      keyBuf += customKey;
      Serial.println(keyBuf);
      keyBufIndex++;
    } else if (keyBufIndex >= 3) {
       keyBuf += customKey;
      //checking to see if entered code is correct
      if (keyBuf.equals(secretCode)) {
        Serial.println("Correct code!");
        toggleLock();
      } else {
        Serial.println("Wrong code!");
      }
      //resetting keybufindex to 0 to start filling new 4 digit code
      keyBufIndex = 0;
      //reset keybuf whether it was correct or not for next code
      keyBuf = "";
    }
    
    //Serial.println(customKey);
  }
  
  // listen for incoming clients
  delay(100);
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    boolean gettingParams = false;
    String inputString = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        switch (c) {
          case '?':
           //at first ? start getting params
          Serial.println("Started gettting params");
          gettingParams = true;
          break;
          
          case '~':
          //recieved final params, end gettingParams
          Serial.println("Ended getting params");
          gettingParams = false;
          parse(inputString);
          authenticate();
          break;
        }
        
        if (c == '\n' && currentLineIsBlank) {
          // change response depending on if user has passed authentication or not
          Serial.println("Sending response");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("Locked status is:");
         if (authenticated) {
           client.print(lockedStatus);
         } else {
          client.print("2"); 
         }
         client.println("<br />");
         client.println("</html>");
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
           // last character on line of received text
           // starting new line with next character read
           currentLineIsBlank = true;
        } else if (c != '\r') {
           // a text character was received from client
           currentLineIsBlank = false;
        }
        
        
        if (gettingParams) {
          inputString += c;
        }
       
      } //end client.available reading
    } //end client.connected
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    authenticated = false;
    Serial.println("client disconnected");
  }

  prevButtonRead = buttonRead;
}

/********************************************************************************/

void parse(String parseString) {
 int startIndex = parseString.indexOf("username");
 int endIndex = parseString.indexOf("&");
 //substring accomodates for the length of the username param
 Serial.print("recieved username: ");
 recievedUsername = parseString.substring(startIndex + 9, endIndex);
 Serial.println(recievedUsername);
 
 startIndex = parseString.indexOf("password");
 endIndex = parseString.indexOf("&", startIndex);
 recievedPassword = parseString.substring(startIndex + 9, endIndex);
 Serial.print("recieved password: ");
 Serial.println(recievedPassword);
 
 startIndex = parseString.indexOf("toggle");
 if (startIndex != -1) {
   //weve recieved a toggle command
   Serial.println("got a toggle command");
   toggling = true;
   endIndex = parseString.indexOf("~", startIndex);
   String recievedAction = parseString.substring(startIndex + 7, endIndex);
   toggleLock();
 } else {
   Serial.println("Just checking status, not toggling");
 }
}

//check recieved credentials
void authenticate() {
  if (recievedUsername == username && recievedPassword == password) {
    authenticated = true;
    Serial.println("authentification passed");
  } else {
    authenticated = false;
    Serial.println("authentification failed");
  }
}

//turn the servo
void toggleLock() {
  Serial.print("Turning servo from old lock status of: ");
  Serial.println(lockedStatus);
  while (!servo.attached()) {}
  if (lockedStatus) {
    servo.write(UNLOCK_POSITION);
  } else {
    servo.write(LOCK_POSITION);
  }
  lockedStatus = !lockedStatus;
  Serial.print("New locked status: ");
  Serial.println(lockedStatus);
}
  

