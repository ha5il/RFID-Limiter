#include "rdm630.h"

#include <SD.h>
#include <SPI.h>               //Serial Peripheral Interface for SD card pins

rdm630 rfid(9, 0);  //TX-pin of RDM630 connected to Arduino pin 9

const int greenLed = 2;             //Leds to Digital 2 3 and 4
const int yellowLed = 3;
const int redLed = 4;
const int blueLed = 5;
const int buzzer = 7;

int i;                          //temp variable for loops
File file;                     // 'file' is File data type (like i is int data type)

const int CS_PIN = 10;               //Only pin that can be varied for SD module

unsigned long oldTime = 0;

bool scanSuccess;

void setup()
{
  pinMode(buzzer, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(yellowLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  //for (i = 1; i < 2; i++)
  // {
  digitalWrite(greenLed, HIGH);
  delay(250);
  digitalWrite(greenLed, LOW);
  digitalWrite(yellowLed, HIGH);
  delay(250);
  digitalWrite(yellowLed, LOW);
  digitalWrite(redLed, HIGH);
  delay(250);
  digitalWrite(redLed, LOW);
  digitalWrite(blueLed, HIGH);
  delay(250);
  digitalWrite(blueLed, LOW);
  digitalWrite(buzzer, HIGH);
  delay(25);
  digitalWrite(buzzer, LOW);
  // }

  Serial.begin(9600);  // start serial to PC
  initializeSD();
  rfid.begin();
}

void loop()
{
  if (SD.exists("data.csv"))
  {
    byte data[6];
    byte length;
    digitalWrite(yellowLed, LOW);       // yellow led is high when card persists

    if (rfid.available())
    {
      scanSuccess = false;
      if ((millis() - oldTime) < 10000)
      {
        digitalWrite(yellowLed, HIGH);
        delay(50);
        Serial.println("Wait 10 sec before rescanning the card!");
      }

      else
      {
        rfid.getData(data, length);
        Serial.println("Data valid");
        for (int i = 0; i < length; i++) {
          Serial.print(data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        //concatenate the bytes in the data array to one long which can be
        //rendered as a decimal number
        unsigned long result =
          ((unsigned long int)data[1] << 24) +
          ((unsigned long int)data[2] << 16) +
          ((unsigned long int)data[3] << 8) +
          data[4];
        Serial.print("CardID: ");
        Serial.println(result);
        String resultt = (String) result;
        scanFile("data.csv", resultt);

        if (scanSuccess)
        {
          digitalWrite(buzzer, HIGH);
          delay(400);
          digitalWrite(buzzer, LOW);
          for (i = 1; i < 3; i++)
          {
            digitalWrite(greenLed, HIGH);
            delay(200);
            digitalWrite(yellowLed, HIGH);
            delay(200);
            digitalWrite(blueLed, HIGH);
            digitalWrite(greenLed, LOW);
            delay(200);
            digitalWrite(yellowLed, LOW);
            delay(200);
            digitalWrite(blueLed, LOW);
          }
        }
        oldTime = millis();
      }
    }
  }
  else
  {
    Serial.println("data not found!");
    digitalWrite(redLed, HIGH);
    delay(500);
    digitalWrite(redLed, LOW);
    delay(500);
  }
}

void initializeSD()
{
  Serial.println("Initializing SD card...");
  pinMode(CS_PIN, OUTPUT);                   // initializeSD is called from setup loop, so setting pin mode

  if (SD.begin())
  {
    Serial.println("SD card is ready to use.");
  }

  else
  {
    Serial.println("SD card initialization failed");
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    return;
  }
}


int scanFile(char filename[], String RFIDvalue)
{
  file = SD.open(filename);
  if (file)
  {
    // Serial.println("File opened with success!");
    while (1)
    {
      String pieces[1]; // String pieces[numberOfPieces];
      int counter = 0;
      int lastIndex = 0;

      String checkline = readLine();

      Serial.print("Starting to check line:");
      Serial.println(checkline);
      if (checkline == "")
      {
        Serial.print("Unknown RFID tag!");
        // logging the unknown rfid to new csv file

        digitalWrite(redLed, HIGH);
        digitalWrite(buzzer, HIGH);
        digitalWrite(blueLed, HIGH);
        digitalWrite(yellowLed, HIGH);
        delay(8000);
        digitalWrite(redLed, LOW);
        digitalWrite(buzzer, LOW);
        digitalWrite(blueLed, LOW);
        digitalWrite(yellowLed, LOW);
        break; //end of file and no rfid tag found!!
      }

      for (int j = 0; j < checkline.length(); j++)
      {
        // Loop through each character and check if it's a comma
        if (checkline.substring(j, j + 1) == ",")
        {
          // Grab the piece from the last index up to the current position and store it
          pieces[counter] = checkline.substring(lastIndex, j);
          // Update the last position and add 1, so it starts from the next character
          lastIndex = j + 1;
          // Increase the position in the array that we store into
          counter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        if (j == checkline.length() - 1)
        {
          // Grab the last part of the string from the lastIndex to the end
          pieces[counter] = checkline.substring(lastIndex, j);
        }
      }
      // checking the rfid in csv
      if (RFIDvalue == pieces[0])
      {
        Serial.println("tag found!");
        // Now verify the scan limit
        File file2;
        file2 = SD.open(pieces[0]);
        if (file2)
        {
          int sd_scan = file2.read() - 48;
          int max_scan = pieces[1].toInt();

          sd_scan++; //making 0th scan 1 so that it works with if and else checks below
          if (max_scan > sd_scan)
          {
            Serial.println("Authorize Washing");
            Serial.print("Value to store in sd: ");
            Serial.println(sd_scan);
            file2.print(sd_scan);
            file2.close();
            scanSuccess = true;
          }
          else if (max_scan == sd_scan)
          {
            Serial.println("Authorize last Washing and take card");
            Serial.print("Value to store in sd: ");
            Serial.println(sd_scan);
            file2.print(sd_scan);
            file2.close();
            scanSuccess = true;
          }
          else
          {
            Serial.println("Take card back without washing");
            Serial.print("Value found while scanning card: ");
            int temp_sd_scan = sd_scan - 1;            // decrementing previous increment
            Serial.println(temp_sd_scan);
            scanSuccess = false;
          }
          break;
        }
        else
        {
          Serial.println("RFID File not found!");
          scanSuccess = false;
        }
      }
    }
    return 1;
  }

  else
  {
    //  Serial.println("Error opening file...");
    return 0;
  }
}

String readLine()
{
  String received = "";
  char ch;
  while (file.available())
  {
    ch = file.read();
    if (ch == '\n')
    {
      return String(received);
    }
    else
    {
      received += ch;
    }
  }
  return "";
}
