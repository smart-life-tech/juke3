
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Wire.h>

const int buzzLedPin = 13; // LED pin for buzz
const int popLedPin = 14;  // LED pin for pop
#define NUM_LEDS_GROUP1 2
#define NUM_LEDS_GROUP2 8
#define NUM_LEDS_GROUP3 10
#define LED_PIN_GROUP1 22
#define LED_PIN_GROUP2 24
#define LED_PIN_GROUP3 32
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
    generateRandomList();
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
    Serial.println("Generating random list of 200 songs...");
    digitalWrite(ledPins[2], LOW);
    digitalWrite(ledPins[1], LOW);
    digitalWrite(ledPins[0], LOW);
    // Generate and print 200 random numbers
    for (int i = 0; i < 200; i++)
    {
        randomSeed(random(analogRead(A0)));

        delay(15);
        randomNumber[i] = random(201); // Generates a random number between 0 and 100
                                       // Serial.println(randomNumber);
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
        sequenceList[sequenceLength] = trackNumber;
        sequenceLength++;
        Serial.print("Track ");
        Serial.print(trackNumber);
        Serial.println(" added to sequence list");
    }
    else
    {
        Serial.println("Sequence list is full");
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
    delay(1000);
    // playIndex++;
    Serial.print("play index number: ");
    Serial.println(playIndex);

    if (playIndex != sequenceLength) // last track?
    {
        Serial.print("sequence total number : ");
        Serial.println(sequenceLength);
        lightUpLEDs(sequenceList[playIndex]);
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

void continuePlaying()
{
    bool busyPinState = digitalRead(busyPin); // read the busy pin

    if (busyPinState == 1 && playIndex == 1 && cancel) // has it gone from low to high?, meaning the track finished
    {

        Serial.print("play number continue 1  = ");
        Serial.println(sequenceList[playIndex]);
        Serial.print("play index continue = ");
        Serial.println(playIndex);
        lightUpLEDs(sequenceList[playIndex]);
        myDFPlayer.play(sequenceList[playIndex]);
        startBuzzPopSequence();
        playIndex++;
    }
    if (busyPinState == 1 && playIndex == 2 && cancel) // has it gone from low to high?, meaning the track finished
    {

        Serial.print("play number continue 2 = ");
        Serial.println(sequenceList[playIndex]);
        Serial.print("play index continue = ");
        Serial.println(playIndex);
        lightUpLEDs(sequenceList[playIndex]);
        myDFPlayer.play(sequenceList[playIndex]);
        startBuzzPopSequence();
        playIndex++;
    }
    if (busyPinState == 1 && playIndex == 3 && cancel) // has it gone from low to high?, meaning the track finished
    {

        Serial.print("play number continue 3  = ");
        Serial.println(sequenceList[playIndex]);
        Serial.print("play index continue = ");
        Serial.println(playIndex);
        lightUpLEDs(sequenceList[playIndex]);
        myDFPlayer.play(sequenceList[playIndex]);
        startBuzzPopSequence();
        playIndex++;
    }
    if (busyPinState == 1 && playIndex == 4 && cancel) // has it gone from low to high?, meaning the track finished
    {
        playSequence();
    }
}

void continuePlayingLong()
{
    if (longPressed)
    {
        bool busyPinState = digitalRead(busyPin); // read the busy pin

        if (busyPinState == 1) // has it gone from low to high?, meaning the track finished
        {

            Serial.print("play number continue   = ");
            Serial.println(randomNumber[playingIndex]);
            Serial.print("play index continue = ");
            Serial.println(playingIndex);
            lightUpLEDs(randomNumber[playingIndex]);
            myDFPlayer.play(randomNumber[playingIndex]);
            startBuzzPopSequence();

            playingIndex++;
        }
    }
}

void playTheList()
{

    static unsigned long timer = 0;
    unsigned long interval = 100;
    if (millis() - timer >= interval)
    {
        timer = millis();

        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        bool busyPinState = digitalRead(busyPin);                           // read the busy pin
        if (busyPinState != lastBusyPinState && playIndex < sequenceLength) // has it changed?
        {
            if (busyPinState == 1) // has it gone from low to high?, meaning the track finished
            {
                Serial.print("playing number  = ");
                Serial.println(sequenceList[playIndex]);
                Serial.print("song index = ");
                Serial.println(playIndex);
                myDFPlayer.stop();
                delay(500);
                lightUpLEDs(sequenceList[playIndex]);
                myDFPlayer.play(sequenceList[playIndex]);
                startBuzzPopSequence();
                playIndex++;                    // next track
                if (playIndex > sequenceLength) // last track?
                {
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
                    sequenceLength--;
                    if (sequenceLength <= 0)
                    {
                        sequenceLength = 0;
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
                Serial.print("still playing next: ");
                Serial.println(playIndex);
            }
            lastBusyPinState = busyPinState; // remember the last busy state

            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        }
        playSequence();
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
        Serial.println(F(" stop the playing"));
        keyBufferIndex = 0;
        Serial.println(F(" skipping the track"));
        skipSequence();

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
            keyBuffer[keyBufferIndex] = key;
            keyBufferIndex++;
            Serial.print("counts: ");
            Serial.println(numCounter);
            numCounter++;
            handleDigitPress();
        }
    }

    row++;
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
    myDFPlayer.volume(20);
    // myDFPlayer.play(3);
    pinMode(busyPin, INPUT);
    // Serial.begin(115200);
    //  Set LED pins as OUTPUT
    for (int i = 0; i < NUM_LEDS_GROUP1; i++)
    {
        pinMode(LED_PIN_GROUP1 + i, OUTPUT);
    }
    for (int i = 0; i < NUM_LEDS_GROUP2; i++)
    {
        pinMode(LED_PIN_GROUP2 + i, OUTPUT);
    }
    for (int i = 0; i < NUM_LEDS_GROUP3; i++)
    {
        pinMode(LED_PIN_GROUP3 + i, OUTPUT);
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
}

void loop()
{
    checking = true;
    // key = keypad.getKey();
    key = getKeypadInput();
    if (key == 'C' && sequenceLength > 1 && keypadLong)
    {
        cancel = true;
        Serial.println(F(" stop the playing"));
        keyBufferIndex = 0;
        Serial.println(F(" skipping the track"));
        skipSequence();

        // playList = false;
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
            delay(2000);
            getEntry('A');
        }
    }
    playTheList();
    updateBuzzPopLeds();
    continuePlayingLong();
}
