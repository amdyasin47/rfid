/* ------------------------------------------------------------------------
 * Created by: Tauseef Ahmad
 * Created on: 17 July, 2022
 *  
 * Tutorial: https://youtu.be/Bgs_3F5rL5Q
 * ------------------------------------------------------------------------
 * Download Resources
 * ------------------------------------------------------------------------
 * Preferences--> Aditional boards Manager URLs : 
 * For ESP8266 and NodeMCU - Board Version 2.6.3
 * http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * ------------------------------------------------------------------------
 * HTTPS Redirect Library:
 * https://github.com/jbuszkie/HTTPSRedirect
 * Example Arduino/ESP8266 code to upload data to Google Sheets
 * Follow setup instructions found here:
 * https://github.com/StorageB/Google-Sheets-Logging
 * ------------------------------------------------------------------------*/

#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
//---------------------------------------------------------------------------------------------------------
// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbxuSUGSm1RvgEQsYMys-KJmPABDI0fLmeyuRhtpvfl0TfXVXgbMY73uxcr4vnPtE7t0_w";
//---------------------------------------------------------------------------------------------------------

// Enter network credentials:
const char *ssid = "";
const char *password = "";
//---------------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"ENT\", \"values\": ";
String payload = "";
//---------------------------------------------------------------------------------------------------------
// Google Sheets setup (do not edit)
const char *host = "script.google.com";
const int httpsPort = 443;
const char *fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect *client = nullptr;
//------------------------------------------------------------
// Declare variables that will be published to Google Sheets
String student_id;
//------------------------------------------------------------
int blocks[] = { 4, 5, 6, 8, 9, 10 };
#define total_blocks (sizeof(blocks) / sizeof(blocks[0]))
//------------------------------------------------------------
#define RST_PIN 0  //D3
#define SS_PIN 2   //D4
#define Buzzer D0
//------------------------------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
//------------------------------------------------------------
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;
/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 20;
byte readBlockData[20];
//--------------------------------------------------

void configModeCallback(WiFiManager *myWiFiManager) {
  digitalWrite(D0, HIGH);
  Serial.println("Entered config mode");
  lcd.init();
  // turn on the backlight
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter config mode");
  lcd.setCursor(4, 1);  //col=0 row=0
  lcd.print("ESP8266");

  WiFi.softAP("ESP8266", "123456789");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());

  delay(500);
  digitalWrite(D0, LOW);
  lcd.clear();
  lcd.setCursor(4, 0);  //col=0 row=0
  lcd.print("ESP8266");
  lcd.setCursor(2, 1);  //col=0 row=0
  lcd.print(WiFi.softAPIP());
}

/****************************************************************************************************
 * setup Function
****************************************************************************************************/
void setup() {
  //------------------------------------------------------
  //Initialize serial communications with PC
  Serial.begin(9600);
  pinMode(D0, OUTPUT);
  pinMode(10, INPUT_PULLUP);
  //initialize lcd screen
  lcd.init();
  // turn on the backlight
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(2, 0);  //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(4, 1);  //col=0 row=0
  lcd.print("WiFi...");
  //------------------------------------------------------
  //  if (digitalRead(10) == LOW) {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect()) {
    Serial.println("fai2 to connect and hit timeout");
    digitalWrite(Buzzer, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);  //col=0 row=0
    lcd.print("fai2 to connect ");
    lcd.setCursor(0, 1);  //col=0 row=0
    lcd.print("and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }
  //  }
  //Initialize SPI bus
  SPI.begin();
  //------------------------------------------------------
  //Initialize MFRC522 Module
  mfrc522.PCD_Init();
  delay(10);
  Serial.println('\n');
  //----------------------------------------------------------
  SPI.begin();
  //----------------------------------------------------------
  //----------------------------------------------------------
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");
  lcd.clear();
  lcd.setCursor(2, 0);  //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(4, 1);  //col=0 row=0
  lcd.print(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    lcd.setCursor(13, 1);  //col=0 row=0
    lcd.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  //----------------------------------------------------------
  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  //----------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print("Google ");
  delay(5000);
  //----------------------------------------------------------
  Serial.print("Connecting to ");
  Serial.println(host);
  //----------------------------------------------------------
  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    //*************************************************
    if (retval == 1) {
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      lcd.clear();
      lcd.setCursor(0, 0);  //col=0 row=0
      lcd.print(msg);
      delay(2000);
      break;
    }
    //*************************************************
    else
      Serial.println("Connection failed. Retrying...");
    //*************************************************
  }
  //----------------------------------------------------------
  if (!flag) {
    //____________________________________________
    lcd.clear();
    lcd.setCursor(0, 0);  //col=0 row=0
    lcd.print("Connection fail");
    //____________________________________________
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(5000);
    return;
    //____________________________________________
  }
  //----------------------------------------------------------
  delete client;     // delete HTTPSRedirect object
  client = nullptr;  // delete HTTPSRedirect object
  //----------------------------------------------------------
  //------------------------------------------------------
}



/****************************************************************************************************
 * loop() function
 ****************************************************************************************************/
void loop() {
  if (analogRead(A0) < 512) {
    setupid();
  } else if ((analogRead(A0) > 512) && (digitalRead(10) == HIGH)) {
    runingsystem();
  }

  if (digitalRead(10) == LOW) {
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect()) {
      Serial.println("fai2 to connect and hit timeout");
      digitalWrite(Buzzer, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);  //col=0 row=0
      lcd.print("fai2 to connect ");
      lcd.setCursor(0, 1);  //col=0 row=0
      lcd.print("and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(1000);
    }
  }
}
void setupid() {
  //------------------------------------------------------------------------------
  // if ((analogRead(A0) > 512) && (digitalRead(10) == HIGH)) { runingsystem(); }

  digitalWrite(Buzzer, HIGH);
  delay(400); 

  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Scan a Your Tag");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print("to write data...");
  delay(100);
  digitalWrite(Buzzer, LOW); 

  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //------------------------------------------------------------------------------
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if (!mfrc522.PICC_IsNewCardPresent()) { return; }
  //------------------------------------------------------------------------------
  /* Select one of the cards */
  if (!mfrc522.PICC_ReadCardSerial()) { return; }
  //------------------------------------------------------------------------------
  Serial.print("\n");
  Serial.println("**Card Detected**");
  digitalWrite(Buzzer, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("**Card Detected**");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print("to write data...");
  delay(1000);
  digitalWrite(Buzzer, LOW);
  /* Print UID of the Card */
  Serial.print(F("Card UID:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("\n");
  /* Print type of card (for example, MIFARE 1K) */
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  //------------------------------------------------------------------------------
  byte buffer[20];
  byte len;
  //wait until 50 seconds for input from serial
  Serial.setTimeout(50000L);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Student ID");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Student ID, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  //add empty spaces to the remaining bytes of buffer
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 4;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("    Data Saved");
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Student Name");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Student Name, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 5;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("    Data Saved");
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Mobile Number");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Mobile Number, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 6;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("    Data Saved");
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Semester");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Semester, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 8;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("    Data Saved");
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Shift");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Shift, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 9;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("    Data Saved");
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer, LOW);
  //MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Enter Tacnology");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print(" ending with #");
  Serial.println(F("---------------------------------------"));
  Serial.println(F("Enter Tacnology, ending with #"));
  len = Serial.readBytesUntil('#', (char *)buffer, 16);
  for (byte i = len; i < 16; i++) buffer[i] = ' ';
  blockNum = 10;
  WriteDataToBlock(blockNum, buffer);
  ReadDataFromBlock(blockNum, readBlockData);
  dumpSerial(blockNum, readBlockData);
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print(" Profile Complete");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print("Remove Card Now");
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  digitalWrite(Buzzer, HIGH);
  delay(500);
  digitalWrite(Buzzer, LOW);
  delay(500);
  ESP.restart();
  delay(1000);
}

void runingsystem() {

  if (analogRead(A0) < 512) {
    setupid();
  }

  if (digitalRead(10) == LOW) {
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect()) {
      Serial.println("fai2 to connect and hit timeout");
      digitalWrite(Buzzer, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);  //col=0 row=0
      lcd.print("fai2 to connect ");
      lcd.setCursor(0, 1);  //col=0 row=0
      lcd.print("and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(1000);
    }
  }
  //----------------------------------------------------------------
  static bool flag = false;
  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr) {
    if (!client->connected()) { client->connect(host, httpsPort); }
  } else {
    Serial.println("Error creating client object!");
  }
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Scan your Tag");

  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if (!mfrc522.PICC_IsNewCardPresent()) { return; }
  /* Select one of the cards */
  if (!mfrc522.PICC_ReadCardSerial()) { return; }
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));
  //----------------------------------------------------------------
  String values = "", data;
  /*
  //creating payload - method 1
  //----------------------------------------------------------------
  ReadDataFromBlock(blocks[0], readBlockData); //student id
  data = String((char*)readBlockData); data.trim();
  student_id = data;
  //----------------------------------------------------------------
  ReadDataFromBlock(blocks[1], readBlockData); //first name
  data = String((char*)readBlockData); data.trim();
  first_name = data;
  //----------------------------------------------------------------
  ReadDataFromBlock(blocks[2], readBlockData); //last name
  data = String((char*)readBlockData); data.trim();
  last_name = data;
  //----------------------------------------------------------------
  ReadDataFromBlock(blocks[3], readBlockData); //phone number
  data = String((char*)readBlockData); data.trim();
  phone_number = data;
  //----------------------------------------------------------------
  ReadDataFromBlock(blocks[4], readBlockData); //address
  data = String((char*)readBlockData); data.trim();
  address = data; data = "";
  //----------------------------------------------------------------
  values = "\"" + student_id + ",";
  values += first_name + ",";
  values += last_name + ",";
  values += phone_number + ",";
  values += address + "\"}";
  //----------------------------------------------------------------*/
  //creating payload - method 2 - More efficient
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    //*************************************************
    if (i == 0) {
      data = String((char *)readBlockData);
      data.trim();
      student_id = data;
      values = "\"" + data + ",";
    }
    //*************************************************
    else if (i == total_blocks - 1) {
      data = String((char *)readBlockData);
      data.trim();
      values += data + "\"}";
    }
    //*************************************************
    else {
      data = String((char *)readBlockData);
      data.trim();
      values += data + ",";
    }
  }
  //----------------------------------------------------------------
  // Create json object string to send to Google Sheets
  // values = "\"" + value0 + "," + value1 + "," + value2 + "\"}"
  payload = payload_base + values;
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0, 0);  //col=0 row=0
  lcd.print("Publishing Data");
  lcd.setCursor(0, 1);  //col=0 row=0
  lcd.print("Please Wait...");
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  //----------------------------------------------------------------
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if (client->POST(url, host, payload)) {
    // do stuff here if publish was successful
    lcd.clear();
    lcd.setCursor(0, 0);  //col=0 row=0
    lcd.print("ID: " + student_id);
    lcd.setCursor(0, 1);  //col=0 row=0
    lcd.print("Student Present");
  }
  //----------------------------------------------------------------
  else {
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
    lcd.clear();
    lcd.setCursor(0, 0);  //col=0 row=0
    lcd.print("Failed.");
    lcd.setCursor(0, 1);  //col=0 row=0
    lcd.print("Try Again");
  }
  //----------------------------------------------------------------
  // a delay of several seconds is required before publishing again
  digitalWrite(Buzzer, HIGH);
  delay(250);
  digitalWrite(Buzzer, LOW);
  delay(250);
  digitalWrite(Buzzer, HIGH);
  delay(250);
  digitalWrite(Buzzer, LOW);
}

/****************************************************************************************************
 * Writ() function
 ****************************************************************************************************/
void WriteDataToBlock(int blockNum, byte blockData[]) {
  //Serial.print("Writing data on block ");
  //Serial.println(blockNum);
  //------------------------------------------------------------------------------
  /* Authenticating the desired data block for write access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Write: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  //------------------------------------------------------------------------------
  else {
    //Serial.print("Authentication OK - ");
  }
  //------------------------------------------------------------------------------
  /* Write data to the block */
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Writing to Block failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    //Serial.println("Write OK");
  }
  //------------------------------------------------------------------------------
}





/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  //Serial.print("Reading data from block ");
  //Serial.println(blockNum);
  //----------------------------------------------------------------------------
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //------------------------------------------------------------------------------
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  //------------------------------------------------------------------------------
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    //Serial.print("Authentication OK - ");
  }
  //------------------------------------------------------------------------------
  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");
  }
  //------------------------------------------------------------------------------
}



/****************************************************************************************************
 * dumpSerial() function
 ****************************************************************************************************/
void dumpSerial(int blockNum, byte blockData[]) {
  Serial.print("\n");
  Serial.print("Data saved on block");
  Serial.print(blockNum);
  Serial.print(": ");
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }
  Serial.print("\n");

  //Empty readBlockData array
  for (int i = 0; i < sizeof(readBlockData); ++i)
    readBlockData[i] = (char)0;  //empty space
}
