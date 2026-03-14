#include <SPI.h>
#include <MFRC522.h>

// --- ESP32 PIN CONNECTIONS ---
#define SS_PIN 5  // RC522 SDA (SS)
#define RST_PIN 2 // RC522 RST

MFRC522 mfrc522(SS_PIN, RST_PIN);  
MFRC522::MIFARE_Key key;

// --- STEP 1: SET THE DATA YOU WANT TO WRITE ---
// Refer to "RFID DATA MAP" in OrientaCart_Silent_Nav.cpp
//
// Examples:
// String dataToWrite = "D_Books";
// String dataToWrite = "D_Electronics";
// String dataToWrite = "J_Books,Elex";
//
String dataToWrite = "D_CHE"; // <-- EDIT THIS LINE FOR EACH TAG
// ---------------------------------------------


// We will write to Block 4 in Sector 1.
byte blockAddr = 4;
byte trailerBlock = 7; // The sector trailer for Sector 1

void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF; // Prepare default key
  }

  Serial.print(F("Ready to write: '"));
  Serial.print(dataToWrite);
  Serial.println(F("' to a MIFARE Classic card."));
  Serial.println(F("Place a card on the reader."));
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(500);
    return;
  }

  Serial.println(F("Card found! Preparing data block..."));

  // --- Prepare the 16-byte data block ---
  byte dataBlock[16];
  // Clear the block with null characters (0x00)
  for (int i=0; i<16; i++) {
    dataBlock[i] = 0x00;
  }
  
  // Copy the string bytes into the block
  // Note: Must not exceed 16 bytes!
  if (dataToWrite.length() > 16) {
    Serial.println("ERROR: Data string is too long (max 16 bytes).");
    return;
  }
  dataToWrite.getBytes(dataBlock, dataToWrite.length() + 1);

  Serial.print("Data to write (HEX): ");
  for(int i=0; i<16; i++) {
    Serial.print(dataBlock[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // --- Authenticate ---
  MFRC522::StatusCode status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // --- Write Data ---
  Serial.print(F("Writing data to Block ")); Serial.print(blockAddr); Serial.println(F("..."));
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println(F("Write successful!"));

  // --- Verify Write ---
  byte readBuffer[18];
  byte size = sizeof(readBuffer);
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, readBuffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  Serial.print(F("Data read back: "));
  for (byte i = 0; i < 16; i++) {
    Serial.write(readBuffer[i]);
  }
  Serial.println();
  Serial.println();

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  Serial.println(F("Data written. Remove card."));
  Serial.println(F("Edit 'dataToWrite' variable for next tag, then upload."));
  while (mfrc522.PICC_IsNewCardPresent()) {
    delay(1000);
  }
}