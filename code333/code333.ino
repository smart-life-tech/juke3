#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Wire.h>
#include <EEPROM.h>
//=================================================================
int musicCount = 0;
int swipeCounter = 0;
int missCounter = 0;
bool swiped = false;
const int interruptPin = A2; // Using pin 2 for interrupt (can be changed)
volatile bool triggerSongSelection = false;
unsigned long lastInterruptTime = 0;
const unsigned long debounceTime = 200; // Debounce for buttons time in milliseconds
//===================================================================
const int buzzLedPin = 13; // LED pin for buzz
const int popLedPin = 14;  // LED pin for pop
#define NUM_LEDS_GROUP1 2
#define NUM_LEDS_GROUP2 10
#define NUM_LEDS_GROUP3 8
#define LED_PIN_GROUP1 22
#define LED_PIN_GROUP2 32
#define LED_PIN_GROUP3 24
int lastPlayed = 0;
const int LONG_PRESS_DURATION = 5000; // 5 seconds (in milliseconds)
unsigned long buttonPressStartTime = 0;
unsigned long pressStartTime = 0;
const unsigned long longPressDuration = 5000; // 5 seconds
bool isPressing = false;
unsigned long keyPressStartTime = 0;
bool isKeyPressed = false;
int oldSequence = 10;
bool done_playing = false;
int row = 16;
bool apressed = false;
bool pause_play = true;
bool playImmediate = false;
byte currentSelection = 1; // Tracks the current selection (1st, 2nd, 3rd)
const byte ROWS = 4;       // four rows
const byte COLS = 4;       // three columns
char keys[ROWS][COLS] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};
const byte busyPin = 10;
const byte ssRXPin = 11;
const byte ssTXPin = 12;
byte playIndex = 0;
const int ledPins[] = {A13, A14, A15}; // LED pins
int keyPressCount = 0;
byte keyBufferIndex = 0;

bool cancel = false;

SoftwareSerial mp3ss(ssRXPin, ssTXPin); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
char key;
char keyBuffer[21];
bool newEntry = false;
byte track = 0;
byte trackList[3] = {0, 0, 0};
byte trackIndex = 0;
byte numTracks = 0;
byte mode = 0;
bool playList = false;
static bool lastBusyPinState = 0;
byte currentDisplayLine = 1;
bool keypadLong = false;
#define MAX_SEQUENCE_LENGTH 100 // Maximum length of the sequence list
bool numAlpha = false;
bool verified = false;
int sequenceList[MAX_SEQUENCE_LENGTH]; // Array to store the sequence list
int sequenceLength = 0;                // Current length of the sequence list
// Function to add a track to the sequence list
bool selectionBlinkState = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkInterval = 500; // Blink interval in milliseconds
bool trackBlinkState = false;
unsigned long lastTrackBlinkTime = 0;
unsigned long trackBlinkInterval = 500; // Blink interval in milliseconds
char collect1 = ' ';
char collect2 = ' ';
int combine = 0;
bool checking = true;
bool buttonEntry = false;
String displayNum[10] = {};
int numCounter = 0;
int randomNumber[200] = {};
bool longPressed = false;
bool lastState[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
int playingIndex = 0;

const char longD = 'Z'; // char returned if long press on 'D'
// const unsigned int longPressDuration = 3000;
unsigned long buzzStartTime = 0;
unsigned long popStartTime = 0;
bool buzzLedOn = false;
bool popLedOn = false;
const int digitPins[10] = {15, 16, 17, 18, 19, 42, 43, 44, 45, 46}; // Pins for digits 0-9
const int resetPin = 51;                                            // Pin for reset (* and #)
const int abcdPins[4] = {47, 48, 49, 50};                           // Pins for A, B, C, D
static unsigned long resetTimer = 0;
unsigned long resetInterval = 30000;
// EEPROM addresses for light show
#define EEPROM_LIGHTSHOW_RUNNING_ADDR 30
#define EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR 31
// Light show state
bool lightShowRunning = false;
unsigned long lightShowEnd = 0;
int lightShowIndex = 0;
unsigned long lastLightShowStep = 0;
const unsigned long lightShowStepDelay = 80;
// Build array of all LED pins for chaser
int allLEDs[NUM_LEDS_GROUP1 + NUM_LEDS_GROUP2 + NUM_LEDS_GROUP3];
bool hasSongStarted = false;
const int inhibitPin = 52; // Pin connected to Nayax inhibit wire
bool inhibitActive = false;

void songSelectionTrigger()
{
    unsigned long currentTime = millis();
    // Debounce
    if (currentTime - lastInterruptTime > debounceTime)
    {
        triggerSongSelection = true;
        lastInterruptTime = currentTime;
        Serial.println(F("External trigger activated for song selection"));
    }
}
// Add this function to control the inhibit
void setInhibit(bool enable)
{
    if (enable)
    {
        digitalWrite(inhibitPin, HIGH); // Pull to 5v to activate inhibit
        inhibitActive = true;
        Serial.println("Nayax inhibit ACTIVATED - card swipes disabled");
    }
    else
    {
        digitalWrite(inhibitPin, LOW); // Release to gndV to disable inhibit
        inhibitActive = false;
        Serial.println("Nayax inhibit DEACTIVATED - card swipes enabled");
    }
}
void splitInteger(int number, char &hundreds, char &tens, char &units)
{
    units = (number % 10) + '0';
    tens = ((number / 10) % 10) + '0';
    hundreds = (number / 100) + '0';
}

void handleLongPress()
{

    // Perform actions for long press
    Serial.println("Long press detected!");
    longPressed = true;
    // lastPlayed = sequenceList[playIndex];
    //  generateRandomList();
    /*int number = (random(201)); // Generates a random number between 0 and 200
      char hundreds, tens, units;
      splitInteger(number, hundreds, tens, units);

      // Print the results
      Serial.print("Hundreds: ");
      Serial.println(hundreds);
      Serial.print("Tens: ");
      Serial.println(tens);
      Serial.print("Units: ");
      Serial.println(units);
      getEntry(hundreds);
      getEntry(tens);
      getEntry(units);
      getEntry('*');*/
    // Add your code to handle the long press here
}

void generateRandomList()
{
    Serial.println("Generating random list of 200 songsss...");
    digitalWrite(ledPins[2], LOW);
    digitalWrite(ledPins[1], LOW);
    digitalWrite(ledPins[0], LOW);

    int i = 0;
    while (i < 200)
    {
        randomSeed(analogRead(A0)); // Reseed the random number generator

        int randomIndex = random(279); // Generate a random number between 0 and 279

        if ((randomIndex > 100 && randomIndex < 180) || (randomIndex > 200 && randomIndex < 279))
        {
            if (randomIndex % 100 != 8 || randomIndex % 100 != 9)
            {
                randomNumber[i] = randomIndex; // Generate number between 100 and 179
                i++;
                if (i > 200)
                    break;
            }
            else
            {
                Serial.print(randomIndex);
                Serial.println('\t');
            }
        }
    }
    for (int i = 0; i < 200; i++)
    {
        Serial.print(randomNumber[i]);
        Serial.print('\t');
    }
}

void startBuzzPopSequence()
{
    buzzLedOn = true;
    buzzStartTime = millis();
    digitalWrite(buzzLedPin, HIGH);
}

void updateBuzzPopLeds()
{
    if (buzzLedOn && millis() - buzzStartTime >= 7000)
    {
        buzzLedOn = false;
        digitalWrite(buzzLedPin, LOW);

        popLedOn = true;
        popStartTime = millis();
        digitalWrite(popLedPin, HIGH);
    }

    if (popLedOn && millis() - popStartTime >= 1000)
    {
        popLedOn = false;
        digitalWrite(popLedPin, LOW);
    }
}

void startLightShow()
{
    // Initialize non-blocking light show (20-LED chaser) for 7 seconds
    Serial.println("Starting 7s light show (chaser animation)");
    lightShowRunning = true;
    EEPROM.write(EEPROM_LIGHTSHOW_RUNNING_ADDR, 1);
    lightShowEnd = millis() + 7000;
    lightShowIndex = 0;
    lastLightShowStep = 0;
    // ensure all LEDs are off to start
    for (int i = 0; i < NUM_LEDS_GROUP1 + NUM_LEDS_GROUP2 + NUM_LEDS_GROUP3; i++)
        digitalWrite(allLEDs[i], LOW);
}

String convertToUpperCase(String input)
{
    char result[input.length() + 1];           // Create character array for result
    input.toCharArray(result, sizeof(result)); // Copy input string to character array

    for (int i = 0; i < input.length(); i++)
    {
        if (isAlpha(result[i]))
        {
            result[i] = toupper(result[i]); // Convert alphabetic character to uppercase
        }
    }

    return String(result); // Convert character array back to String and return
}

bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void testled()
{
    for (int i = 0; i < NUM_LEDS_GROUP1; i++)
    {
        digitalWrite(LED_PIN_GROUP1 + i, HIGH);
        //  delay(200);
    }
    for (int i = 0; i < NUM_LEDS_GROUP2; i++)
    {
        digitalWrite(LED_PIN_GROUP2 + i, HIGH);
        //  delay(200);
    }
    for (int i = 0; i < NUM_LEDS_GROUP3; i++)
    {
        digitalWrite(LED_PIN_GROUP3 + i, HIGH);
        // delay(200);
    }
    // delay(2000);
    for (int i = 0; i < NUM_LEDS_GROUP1; i++)
    {
        digitalWrite(LED_PIN_GROUP1 + i, LOW);
    }
    for (int i = 0; i < NUM_LEDS_GROUP2; i++)
    {
        digitalWrite(LED_PIN_GROUP2 + i, LOW);
    }
    for (int i = 0; i < NUM_LEDS_GROUP3; i++)
    {
        digitalWrite(LED_PIN_GROUP3 + i, LOW);
    }
}

void lightUpLEDs(int trackNumber)
{
    String trackString = String(trackNumber); // Convert track number to a string
    trackString = String("000") + trackString;
    trackString = trackString.substring(trackString.length() - 3);
    // Turn off all LEDs
    Serial.println(trackString);
    for (int i = 0; i < NUM_LEDS_GROUP1; i++)
    {
        digitalWrite(LED_PIN_GROUP1 + i, LOW);
    }
    for (int i = 0; i < NUM_LEDS_GROUP2; i++)
    {
        digitalWrite(LED_PIN_GROUP2 + i, LOW);
    }
    for (int i = 0; i < NUM_LEDS_GROUP3; i++)
    {
        digitalWrite(LED_PIN_GROUP3 + i, LOW);
    }

    // Activate LEDs for each group based on the track number
    if (trackNumber > 99)
        digitalWrite(LED_PIN_GROUP1 + trackString.substring(0, 1).toInt() - 1, HIGH);
    // Group 2 LEDs
    if (trackNumber > 9)
        digitalWrite(LED_PIN_GROUP2 + trackString.substring(1, 2).toInt(), HIGH);

    // Group 1 LEDs
    // if (trackNumber > 0)
    digitalWrite(LED_PIN_GROUP3 + trackString.substring(2, 3).toInt(), HIGH);

    Serial.print(trackString.substring(0, 1).toInt());
    Serial.print(trackString.substring(1, 2).toInt());
    Serial.print(trackString.substring(2, 3).toInt());
    Serial.println(" done");
}

void addToSequenceList(int trackNumber)
{
    if (sequenceLength < MAX_SEQUENCE_LENGTH)
    {
        if (trackNumber % 100 != 8 && trackNumber % 100 != 9)
        {
            sequenceList[sequenceLength] = trackNumber;
            sequenceLength++;
            musicCount++;
            Serial.print(sequenceLength);
            Serial.print("  Track ");
            Serial.print(trackNumber);
            Serial.println(" added to sequence list");
            hasSongStarted = true;
        }
        else
        {
            Serial.println("enter track not ending in 8 or 9 !!!");
        }
    }
    else
    {
        Serial.println("Sequence list is full");
    }
}

void checkReset()
{
    if (digitalRead(busyPin) == 0)
        resetTimer = millis();
    // Check if 30 seconds have passed since the last song ended
    if (musicCount <= 0 && digitalRead(busyPin) == 1 && hasSongStarted)
    {
        if (millis() - resetTimer > resetInterval)
        {
            // Reset logic
            Serial.println("Resetting due to inactivity...");
            sequenceLength = 0;
            playIndex = 0;      // reset list
            keyBuffer[0] = 'C'; // set up for stop mode
            mode = 6;           // call stop mode
            playList = false;
            cancel = false;
            resetTimer = millis();
            musicCount = 0;
            for (int i = 0; i < NUM_LEDS_GROUP1; i++)
            {
                digitalWrite(LED_PIN_GROUP1 + i, LOW);
            }
            for (int i = 0; i < NUM_LEDS_GROUP2; i++)
            {
                digitalWrite(LED_PIN_GROUP2 + i, LOW);
            }
            for (int i = 0; i < NUM_LEDS_GROUP3; i++)
            {
                digitalWrite(LED_PIN_GROUP3 + i, LOW);
            }
            asm volatile("jmp 0x0000");
        }
    }
}

// Function to play the sequence
void playSequence()
{
    if (digitalRead(busyPin) == 1 && (playIndex == 4 || done_playing))
    { // has it gone from low to high?, meaning the track finished
        asm volatile("jmp 0x0000");
    }
}
// Function to stop the sequence
void stopSequence()
{
    // playList = true;
    keyBufferIndex = 0;
    myDFPlayer.pause();
    Serial.println("paused");
}

void skipSequence()
{
    myDFPlayer.stop();
    delay(1000);
    // playIndex--;
    Serial.print("play index number: ");
    Serial.println(playIndex);
    Serial.print(" number 1: ");
    Serial.println(sequenceList[0]);
    Serial.print(" number 2: ");
    Serial.println(sequenceList[1]);
    Serial.print(" number 3: ");
    Serial.println(sequenceList[2]);

    if (playIndex != sequenceLength) // last track?
    {
        Serial.print("sequence skipping total number : ");
        Serial.println(sequenceLength);
        lightUpLEDs(sequenceList[playIndex]);
        Serial.print("sequence skipping, playing : ");
        Serial.println(sequenceList[playIndex]);
        myDFPlayer.play(sequenceList[playIndex]);
        startBuzzPopSequence();
        playIndex++;
        playList = false;
    }
    else
    {
        stopSequence();
    }
}

void skipSeq()
{
    myDFPlayer.stop();
    Serial.println("skipped");
    delay(1000);
}

void continuePlaying(int play)
{
    bool busyPinState = digitalRead(busyPin);             // read the busy pin
    if (busyPinState == 1 && playIndex == play && cancel) // has it gone from low to high?, meaning the track finished
    {

        Serial.print("play number after skip  = ");
        Serial.println(sequenceList[playIndex]);
        Serial.print("play index continue = ");
        Serial.println(playIndex);
        lightUpLEDs(sequenceList[playIndex]);
        Serial.print("sequence skipping, playing : ");
        Serial.println(sequenceList[playIndex]);
        myDFPlayer.play(sequenceList[playIndex]);
        startBuzzPopSequence();
        playIndex++;
        if (playIndex > sequenceLength) // last track?
        {
            // sequenceLength = 0;
            playIndex = 0;      // reset list
            keyBuffer[0] = 'C'; // set up for stop mode
            mode = 6;           // call stop mode
            playList = false;
            cancel = false;
            for (int i = 0; i < NUM_LEDS_GROUP1; i++)
            {
                digitalWrite(LED_PIN_GROUP1 + i, LOW);
            }
            for (int i = 0; i < NUM_LEDS_GROUP2; i++)
            {
                digitalWrite(LED_PIN_GROUP2 + i, LOW);
            }
            for (int i = 0; i < NUM_LEDS_GROUP3; i++)
            {
                digitalWrite(LED_PIN_GROUP3 + i, LOW);
            }
        }
        else
        {
            musicCount--;
            if (musicCount < 0)
            {
                musicCount = 0;
                digitalWrite(ledPins[2], LOW);
                digitalWrite(ledPins[1], LOW);
                digitalWrite(ledPins[0], LOW);
            }
        }
        if (sequenceLength == 0)
        {
            done_playing = true;
            delay(1000);
        }
    }
}

void continuePlayingLong()
{

    if (longPressed)
    {

        bool busyPinState = digitalRead(busyPin); // read the busy pin
        digitalWrite(A15, HIGH);
        if (digitalRead(busyPin) == 1) // has it gone from low to high?, meaning the track finished
        {
            Serial.println("song not selected starting a new song now");
            String trackString = String(lastPlayed); // Convert track number to a string
            trackString = String("000") + trackString;
            trackString = trackString.substring(trackString.length() - 3);
            Serial.print("playing :");
            Serial.println(trackString);
            Serial.print("playing endings :");
            Serial.println(trackString.substring(2, 3).toInt());

            if (lastPlayed >= 100 && lastPlayed < 298)
            {
                if ((trackString.substring(2, 3).toInt() != 8) && (trackString.substring(2, 3).toInt() != 9))
                {
                    if (lastPlayed >= 298)
                    {
                        lastPlayed = 100;
                        myDFPlayer.stop();
                        Serial.println("last played reset to 100");
                        delay(500);
                        myDFPlayer.play(lastPlayed);
                    }
                    Serial.print("playing number in continous mode = ");
                    Serial.println(lastPlayed);
                    // myDFPlayer.stop();
                    // delay(500);
                    lightUpLEDs(lastPlayed);
                    delay(2500);
                    myDFPlayer.play(lastPlayed);
                    delay(500);
                    lastPlayed++;

                    Serial.print("playing number in readstate2 = ");
                    Serial.println(myDFPlayer.readState());
                    Serial.print("  ");
                    delay(50);
                    startBuzzPopSequence();
                }
                else
                {
                    Serial.println("number ended in a 8 or 9");
                    lastPlayed++;
                }
            }
            else
            {
                Serial.print("un recongized number = ");
                Serial.println(lastPlayed);
                if (lastPlayed == 180)
                    lastPlayed = 200;
                if (lastPlayed >= 277)
                {
                    lastPlayed = 100;
                    myDFPlayer.stop();
                    Serial.println("last played reset to 100");
                    delay(500);
                    myDFPlayer.play(lastPlayed);
                }
            }
        }
    }
}

void playTheList()
{
    static unsigned long timer = 0;
    unsigned long interval = 500;
    if (millis() - timer >= interval)
    {
        timer = millis();
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        bool busyPinState = digitalRead(busyPin);                           // read the busy pin
        if (busyPinState != lastBusyPinState && playIndex < sequenceLength) // has it changed?
        {
            if (busyPinState == 1 && !keypadLong) // has it gone from low to high?, meaning the track finished
            {
                lastPlayed = sequenceList[playIndex];
                if ((lastPlayed >= 100 && lastPlayed < 180) || (lastPlayed >= 180 && lastPlayed < 298))
                {
                    Serial.print("playing number  = ");
                    Serial.println(sequenceList[playIndex]);
                    Serial.print("song index = ");
                    Serial.println(playIndex);
                    myDFPlayer.stop();
                    delay(500);
                    lightUpLEDs(sequenceList[playIndex]);
                    Serial.print(" playing the list: ");
                    Serial.println(sequenceList[playIndex]);
                    
                    // Trigger light show for 2nd or 3rd song (playIndex 1 or 2)
                    if (playIndex == 1 || playIndex == 2)
                    {
                        Serial.print("Triggering light show for song index: ");
                        Serial.println(playIndex);
                        startLightShow();
                    }
                    
                    myDFPlayer.play(sequenceList[playIndex]);
                    startBuzzPopSequence();
                    lastPlayed = sequenceList[playIndex];
                    playIndex++; // next track
                    lastPlayed++;
                    // Reset the reset timer
                    resetTimer = millis();
                    if (playIndex > sequenceLength) // last track?
                    {
                        setInhibit(false);
                        sequenceLength = 0;
                        playIndex = 0;      // reset list
                        keyBuffer[0] = 'C'; // set up for stop mode
                        mode = 6;           // call stop mode
                        playList = false;
                        cancel = false;
                        for (int i = 0; i < NUM_LEDS_GROUP1; i++)
                        {
                            digitalWrite(LED_PIN_GROUP1 + i, LOW);
                        }
                        for (int i = 0; i < NUM_LEDS_GROUP2; i++)
                        {
                            digitalWrite(LED_PIN_GROUP2 + i, LOW);
                        }
                        for (int i = 0; i < NUM_LEDS_GROUP3; i++)
                        {
                            digitalWrite(LED_PIN_GROUP3 + i, LOW);
                        }
                    }
                    else
                    {
                        musicCount--;
                        setInhibit(true);
                        Serial.print("music count : ");
                        Serial.println(musicCount);
                        if (musicCount < 0)
                        {
                            // musicCount = 0;
                            digitalWrite(ledPins[2], LOW);
                            digitalWrite(ledPins[1], LOW);
                            digitalWrite(ledPins[0], LOW);
                        }
                    }
                    if (playIndex == 3)
                    {
                        done_playing = true;
                        delay(1000);
                    }
                    Serial.print("song playing next: ");
                    Serial.println(playIndex);
                } // here
                else
                {
                    Serial.print("un recongized number = ");
                    Serial.println(lastPlayed);
                }
            }
            lastBusyPinState = busyPinState; // remember the last busy state

            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        }
        checkReset();
        playSequence();
        
        // Handle chaser animation if running
        if (lightShowRunning)
        {
            unsigned long now = millis();
            if (now - lastLightShowStep >= lightShowStepDelay)
            {
                // turn all LEDs off
                for (int i = 0; i < NUM_LEDS_GROUP1 + NUM_LEDS_GROUP2 + NUM_LEDS_GROUP3; i++)
                    digitalWrite(allLEDs[i], LOW);

                // light the current index
                digitalWrite(allLEDs[lightShowIndex], HIGH);

                lightShowIndex = (lightShowIndex + 1) % (NUM_LEDS_GROUP1 + NUM_LEDS_GROUP2 + NUM_LEDS_GROUP3);
                lastLightShowStep = now;
            }

            // end the show when time's up
            if (millis() >= lightShowEnd)
            {
                lightShowRunning = false;
                Serial.println("Light show ended.");
                EEPROM.write(EEPROM_LIGHTSHOW_RUNNING_ADDR, 0);
                // restore LEDs for current song display
                lightUpLEDs(lastPlayed);
            }
        }
    }
}

String intToAlphanumeric(int number)
{
    // Define a string to store the alphanumeric combination
    String result = "";

    // Calculate the letter part of the combination ('A' for 1, 'B' for 2, etc.)
    char letter = 'A' + ((number - 1) / 10);

    // Calculate the digit part of the combination ('1' for 1, '2' for 2, etc.)
    char digit = '0' + ((number - 1) % 10) + 1;

    // Concatenate the letter and digit to form the alphanumeric combination
    result += letter;
    result += digit;

    return result;
}

void getEntry(char key)
{
    static boolean entryStarted = false;
    if (key == 'C' && sequenceLength > 1)
    {
        cancel = true;
        playList = false;
        Serial.println(F(" stop the playing"));
        keyBufferIndex = 0;
        Serial.println(F(" skipping the track"));
        skipSeq();

        // playList = false;
    }
    if (key == 'C' && playImmediate)
    {
        cancel = true;
        Serial.println(F(" stop the playings"));
        asm volatile("jmp 0x0000");

        // playList = false;
    }
    if (key == '#')
    {

        if (pause_play)
        {
            Serial.println(F(" pause the playing"));
            myDFPlayer.pause();
            pause_play = false;
        }
        else
        {
            if (playList || cancel)
            {
                Serial.println(F(" continue playing  the track"));
                myDFPlayer.start();
                pause_play = true;
                delay(1000);
            }
        }
        // pause_play = !pause_play;
    }

    if (key == 'D')
    { // Delete last key entry
        if (keyBufferIndex > 0)
        {
            memset(keyBuffer, 0, sizeof(keyBuffer));
            keyBufferIndex = 0;
            row = 16;
            // Serial.print(F("\t\t"));
            Serial.print(sequenceList[sequenceLength]);
            Serial.println(F(" deleted track"));

            // Reset display
            // sequenceList[sequenceLength] = trackNumber;
            // sequenceLength--;
            // return;
        }
    }
    else if (key == '*' || key == 'A' || key == 'B' || key == 'C')
    { // Action keys
        if (keyBufferIndex > 0)
        { // Check if a track number has been entered
            switch (key)
            {
            case '*': // Play immediate
                Serial.println(F(" play immediate"));
                playImmediate = true;
                lightUpLEDs(atoi(keyBuffer));
                myDFPlayer.play(atoi(keyBuffer)); // Assuming track numbers are in folder 1
                startBuzzPopSequence();
                keyBufferIndex = 0;
                delay(1000);
                playIndex = 4;
                cancel = true;
                memset(keyBuffer, 0, sizeof(keyBuffer));
                break;
            case 'A': // Add to sequence list
                Serial.println(F(" add to list"));
                addToSequenceList(atoi(keyBuffer));
                // Clear the buffer
                confirmSelection();
                memset(keyBuffer, 10, sizeof(keyBuffer));
                // keyBufferIndex = 0;
                break;
            case 'B': // Play sequence
                Serial.println(F(" playing the sequence"));
                playList = true;

                keyBufferIndex = 0;
                // playSequence();
                break;
            case 'C': // STOP sequence
                Serial.println(F(" stop the playings"));
                keyBufferIndex = 0;
                stopSequence();
                break;
            default:
                break;
            }
        }
    }

    else if (key != '*' && key != 'D')
    { // Continue entry
        if (keyBufferIndex < 9 && isDigit(key))
        {
            Serial.print("key entered: ");
            Serial.println(key);
            //.print("keyBufferIndex: ");
            // Serial.println(keyBufferIndex);
            // Serial.print("keyBuffer: ");
            // Serial.println(keyBuffer);
            // Serial.print("keyBuffer[keyBufferIndex]: ");
            // Serial.println(keyBuffer[keyBufferIndex]);
            Serial.print("num counter: ");
            Serial.println(numCounter);
            if (numCounter == 0 && (key == '1' || key == '2'))
            {
                doEntry(key);
            }
            else if (numCounter == 1) //&& (key != '8' || key != '9'))
            {                         // only accept 0 to 7  at second press
                                      // if (key == '0' || key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7')
                                      //  {
                doEntry(key);
                //}
            }
            else if (numCounter == 2)
            {
                if (key == '0' || key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7')
                {
                    doEntry(key);
                }
            }
        }
    }

    row++;
}

void doEntry(char keys)
{
    keyBuffer[keyBufferIndex] = keys;
    keyBufferIndex++;
    Serial.print("counts: ");
    Serial.println(numCounter);
    numCounter++;
    handleDigitPress();
    verified = true;
    // if (numCounter >= 3)
    //{
    //   numCounter = 0;
    //  verified = false;
    // }
}

float entryToFloat(char *entry)
{
    return (atof(entry));
}

int entryToInt(char *entry)
{
    return (atoi(entry));
}

void handleDigitPress()
{
    keyPressCount++;

    if (keyPressCount == 1)
    {
        digitalWrite(ledPins[0], HIGH); // Turn on the first LED
    }
    else if (keyPressCount == 2)
    {
        digitalWrite(ledPins[1], HIGH); // Turn on the second LED
    }
    else if (keyPressCount == 3)
    {
        digitalWrite(ledPins[0], LOW);  // Turn off the first LED
        digitalWrite(ledPins[1], LOW);  // Turn off the second LED
        digitalWrite(ledPins[2], HIGH); // Turn on the third LED
    }
}

void confirmSelection()
{
    if (keyPressCount == 3)
    {
        digitalWrite(ledPins[2], LOW); // Turn off the third LED
        keyPressCount = 0;             // Reset the key press count
    }
}

char getKeypadInput()
{
    for (int i = 0; i < 10; i++)
    {
        if (digitalRead(digitPins[i]) == LOW)
        {
            delay(10); // Debounce delay
            if (digitalRead(digitPins[i]) == LOW)
            { // Confirm button press

                Serial.println("button pressed");
                // Handle long press for '0'
                if (!isKeyPressed)
                {
                    Serial.println("do once");
                    keyPressStartTime = millis();
                    isKeyPressed = true;
                }

                while (digitalRead(digitPins[i]) == LOW)
                {
                    if (millis() - keyPressStartTime >= 5000)
                    {
                        // generateRandomList();
                        handleLongPress();
                        keypadLong = true;
                        Serial.print(F("keypad long pressed"));
                        isKeyPressed = false;
                        return '\0';
                        break;
                    }
                }

                // Wait until button is released
                isKeyPressed = false;
                return '0' + i;
            }
        }
    }

    if (digitalRead(resetPin) == LOW)
    {
        delay(50); // Debounce delay
        if (digitalRead(resetPin) == LOW)
        { // Confirm button press
            while (digitalRead(resetPin) == LOW)
                ;       // Wait until button is released
            return '#'; // Use # as reset key
        }
    }

    for (int i = 0; i < 4; i++)
    {
        if (digitalRead(abcdPins[i]) == LOW)
        {
            delay(50); // Debounce delay
            if (digitalRead(abcdPins[i]) == LOW)
            { // Confirm button press
                while (digitalRead(abcdPins[i]) == LOW)
                    ; // Wait until button is released
                return 'A' + i;
            }
        }
    }

    return '\0'; // No key pressed
}

void setup()
{
    Serial.begin(115200);
    Serial.print(F("Enter track number then enter action"));
    Serial.println(F(" # = ENTER"));
    Serial.println(F(" * = Play immediate"));
    Serial.println(F(" A = Add to sequence list"));
    Serial.println(F(" B = Play sequence"));
    Serial.println(F("To add track 18 to the list, \"18A\""));
    Serial.println(F("To play track 3 immediately, \"3*\""));
    Serial.println(F(" C = STOP sequence"));
    Serial.println(F("\n\n"));
    mp3ss.begin(9600);
    myDFPlayer.begin(mp3ss);
    myDFPlayer.volume(25);
    // myDFPlayer.play(3);
    pinMode(busyPin, INPUT);
    pinMode(interruptPin, INPUT);
    // Initialize inhibit pin
    pinMode(inhibitPin, OUTPUT);
    digitalWrite(inhibitPin, LOW); // Start with inhibit disabled (0v)
    Serial.println("inhibit started with disabled");
    //  Set LED pins as OUTPUT
    for (int i = 0; i < NUM_LEDS_GROUP1; i++)
    {
        pinMode(LED_PIN_GROUP1 + i, OUTPUT);
        allLEDs[i] = LED_PIN_GROUP1 + i;
    }
    for (int i = 0; i < NUM_LEDS_GROUP2; i++)
    {
        pinMode(LED_PIN_GROUP2 + i, OUTPUT);
        allLEDs[NUM_LEDS_GROUP1 + i] = LED_PIN_GROUP2 + i;
    }
    for (int i = 0; i < NUM_LEDS_GROUP3; i++)
    {
        pinMode(LED_PIN_GROUP3 + i, OUTPUT);
        allLEDs[NUM_LEDS_GROUP1 + NUM_LEDS_GROUP2 + i] = LED_PIN_GROUP3 + i;
    }
    for (int i = 0; i < 3; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW); // Ensure all LEDs are off initially
    }
    testled();
    for (int i = 0; i < 10; i++)
    {
        pinMode(digitPins[i], INPUT_PULLUP);
    }
    pinMode(resetPin, INPUT_PULLUP);

    for (int i = 0; i < 4; i++)
    {
        pinMode(abcdPins[i], INPUT_PULLUP);
    }
    Serial.println(F(" C = STOP sequence"));
    // Initialize random number generator
    //  randomSeed(analogRead(0));
    pinMode(buzzLedPin, OUTPUT);
    digitalWrite(buzzLedPin, LOW); // Ensure buzz LED is off initially
    pinMode(popLedPin, OUTPUT);
    digitalWrite(popLedPin, LOW); // Ensure pop LED is off initially
    // myDFPlayer.play(100);         // play the 100 track at start to check
    digitalWrite(inhibitPin, LOW); // Start with inhibit disabled (0v)
}

void loop()
{
    while (!swiped)
    {
        int pinValue = analogRead(interruptPin);
        Serial.print("pin value = ");
        Serial.println(pinValue);
        delay(500);

        if (pinValue <= 0)
        {
            swipeCounter++;
            missCounter = 0; // reset misses since we got a valid read

            if (swipeCounter >= 1)
            {
                Serial.println("card SWIPING OCCURRED now on pin");
                Serial.print("pin value = ");
                Serial.println(pinValue);
                delay(500);
                swiped = true;
                break;
            }
        }
        else
        {
            missCounter++;
            if (missCounter >= 10)
            {
                // Serial.println("Too many invalid reads. Resetting swipeCounter.");
                swipeCounter = 0;
                missCounter = 0;
            }
        }
    }
    if (swiped)
    {
        checking = true;
        //digitalWrite(inhibitPin, HIGH);
        // key = keypad.getKey();
        key = getKeypadInput();
        if (key == 'C' && sequenceLength > 1 && keypadLong)
        {
            cancel = true;
            Serial.println(F(" stop the playing"));
            keyBufferIndex = 0;
            Serial.println(F(" skipping the track"));
            skipSeq();
            // delay(2000);
            //  playList = false;
        }
        if (key == 'C' && longPressed)
        {
            skipSeq();
        }
        if (key && !keypadLong)
        {
            isPressing = false; // Reset if another key is pressed
            Serial.print(F(" key code = "));
            Serial.println(key);
            if (key == 'Z')
            {
                handleLongPress();
                keypadLong = true;
                Serial.print(F("keypad long pressed"));
                digitalWrite(ledPins[0], LOW); // Turn off the first LED
                digitalWrite(ledPins[1], LOW); // Turn off the second LED
                digitalWrite(ledPins[2], LOW); // Turn OFF the third LED
            }
            getEntry(key);
            if (numCounter >= 3)
            {
                numCounter = 0;
                delay(800);
                getEntry('A');
                verified = false;
            }
        }
        playTheList();
        updateBuzzPopLeds();
        continuePlayingLong();
    }
}