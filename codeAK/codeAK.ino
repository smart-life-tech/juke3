#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <EEPROM.h>

#define NUM_LETTERS 10
#define NUM_NUMBERS 10
#define MAX_TRACKS 100
#define MAX_QUEUE 3
int currentLetter = -1;
int currentNumber = -1;
int lastPlayedLetter = -1;
int lastPlayedNumber = -1;
bool continuousPlay = false;
int startLetter = -1;
int startNumber = -1;
int contLetter = -1;
int contNumber = -1;
unsigned long pressStartTime = 0;
bool isLetterPressed = false;
int currentPressedLetter = -1;
unsigned long blinkStart = 0;
bool ledsOn = true;
const int busyPin = 12;
const int skipPin = 13;
const int buzzLedPin = 14;
const int popLedPin = 15;
bool skip = false;
bool donePlaying = true;
bool play = false;
unsigned long buzzStartTime = 0;
unsigned long popStartTime = 0;
bool buzzLedOn = false;
bool popLedOn = false;
unsigned long lastSkipDebounce = 0;
// Letter button pins A–K (skipping I)
int letterPins[NUM_LETTERS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
// Number button pins 0–9
int numberPins[NUM_NUMBERS] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
// LED pins for letters and numbers
int letterLEDs[NUM_LETTERS] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
int numberLEDs[NUM_NUMBERS] = {42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

SoftwareSerial mp3Serial(16, 17); // RX, TX
DFRobotDFPlayerMini mp3;
int oldPlay = -1;
char letters[NUM_LETTERS] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K'};

struct Song
{
    int letter;
    int number;
};

Song queue[MAX_QUEUE];
int queueSize = 0;
int currentPlaying = 0;

unsigned long lastLetterDebounce = 0;
unsigned long lastNumberDebounce = 0;
const unsigned long debounceDelay = 50;

// EEPROM addresses
#define EEPROM_QUEUE_START 0
#define EEPROM_QUEUE_SIZE_ADDR 6
#define EEPROM_CURRENT_PLAYING_ADDR 7
#define EEPROM_RESET_FLAG_ADDR 8
#define EEPROM_CONTINUOUS_PLAY_ADDR 9
#define EEPROM_START_LETTER_ADDR 10
#define EEPROM_START_NUMBER_ADDR 11
#define EEPROM_CONT_LETTER_ADDR 12
#define EEPROM_CONT_NUMBER_ADDR 13

// Reset function
void (*resetFunc)(void) = 0;

void saveQueue()
{
    for (int i = 0; i < MAX_QUEUE; i++)
    {
        EEPROM.write(EEPROM_QUEUE_START + i * 2, queue[i].letter);
        EEPROM.write(EEPROM_QUEUE_START + i * 2 + 1, queue[i].number);
    }
    EEPROM.write(EEPROM_QUEUE_SIZE_ADDR, queueSize);
    EEPROM.write(EEPROM_CURRENT_PLAYING_ADDR, currentPlaying);
}

void loadQueue()
{
    for (int i = 0; i < MAX_QUEUE; i++)
    {
        queue[i].letter = EEPROM.read(EEPROM_QUEUE_START + i * 2);
        queue[i].number = EEPROM.read(EEPROM_QUEUE_START + i * 2 + 1);
    }
    queueSize = EEPROM.read(EEPROM_QUEUE_SIZE_ADDR);
    currentPlaying = EEPROM.read(EEPROM_CURRENT_PLAYING_ADDR);
    Serial.print("Loaded queue size from EEPROM: ");
    Serial.println(queueSize);
    Serial.print("Loaded current playing index from EEPROM: ");
    Serial.println(currentPlaying);
    Serial.println();
}

void clearEEPROM()
{
    for (int i = 0; i < 14; i++)
    {
        EEPROM.write(i, 0);
    }
}

void setup()
{
    Serial.begin(9600);
    mp3Serial.begin(9600);
    if (!mp3.begin(mp3Serial))
    {
        Serial.println("DFPlayer Mini not found!, proceeding regardless, no effect");
        // while (true)
        //     ;
        // mp3.play(1);
    }
    mp3.setTimeOut(50); // Set timeout to prevent hangs
    mp3.volume(20);     // Set volume

    for (int i = 0; i < NUM_LETTERS; i++)
    {
        pinMode(letterPins[i], INPUT_PULLUP);
        pinMode(letterLEDs[i], OUTPUT);
        digitalWrite(letterLEDs[i], HIGH); // all ON initially
    }
    for (int i = 0; i < NUM_NUMBERS; i++)
    {
        pinMode(numberPins[i], INPUT_PULLUP);
        pinMode(numberLEDs[i], OUTPUT);
        digitalWrite(numberLEDs[i], HIGH);
    }

    pinMode(busyPin, INPUT);
    pinMode(skipPin, INPUT_PULLUP);
    pinMode(buzzLedPin, OUTPUT);
    digitalWrite(buzzLedPin, LOW);
    pinMode(popLedPin, OUTPUT);
    digitalWrite(popLedPin, LOW);

    // Check reset flag
    int resetFlag = EEPROM.read(EEPROM_RESET_FLAG_ADDR);
    if (resetFlag == 1)
    {
        // Software reset: clear flag and proceed normally
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
        Serial.println("Software reset detected, clearing flag.");
    }
    else
    {
        // Hardware reset: clear EEPROM
        clearEEPROM();
        Serial.println("Hardware reset detected, clearing EEPROM.");
    }

    // Load continuous play state
    continuousPlay = EEPROM.read(EEPROM_CONTINUOUS_PLAY_ADDR);
    if (continuousPlay)
    {
        startLetter = EEPROM.read(EEPROM_START_LETTER_ADDR);
        startNumber = EEPROM.read(EEPROM_START_NUMBER_ADDR);
        contLetter = EEPROM.read(EEPROM_CONT_LETTER_ADDR);
        contNumber = EEPROM.read(EEPROM_CONT_NUMBER_ADDR);
        Serial.println("Resuming continuous play from EEPROM.");
        playSong(contLetter, contNumber);
        play = true;
        delay(500);
    }
    else
    { // Load queue from EEPROM
        loadQueue();
        if (currentPlaying == 3 && currentPlaying > queueSize)
        {
            EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
            Serial.println("Queue finished, resetting.");
            delay(500);
            resetFunc();
        }
        if (queueSize == 3 && currentPlaying < queueSize)
        {
            Serial.println("Resuming playback from EEPROM.");
            playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
            play = true;
            currentPlaying++;
            saveQueue(); // Save after resuming
            delay(500);  // brief delay to allow mp3 module to start
        }
    }
    // queueSize = 0;
    // currentPlaying = 0;

    Serial.println("Code AK Ready! v1.18");
}

void loop()
{
    int letterPressed = getPressedKey(letterPins, NUM_LETTERS);
    int numberPressed = getPressedKey(numberPins, NUM_NUMBERS);

    // Handle letter press for longpress detection
    if (letterPressed != -1 && !isLetterPressed)
    {
        isLetterPressed = true;
        currentPressedLetter = letterPressed;
        pressStartTime = millis();
    }
    else if (letterPressed == -1 && isLetterPressed)
    {
        unsigned long pressDuration = millis() - pressStartTime;
        if (pressDuration > 1500)
        {
            Serial.println("Long letter press detected");
            // Longpress: start continuous play from currently playing song
            if (play && lastPlayedLetter != -1 && lastPlayedNumber != -1)
            {
                Serial.print("Starting continuous play from last played: ");
                Serial.print(letters[lastPlayedLetter]);
                Serial.println(lastPlayedNumber);
                delay(1000);

                startContinuousPlay(lastPlayedLetter, lastPlayedNumber);
            }
            else
            {
                Serial.println("Cannot start continuous play - no song playing");
            }
        }
        else if (pressDuration <= 1000 && millis() - lastLetterDebounce > debounceDelay)
        {
            lastLetterDebounce = millis();
            handleLetterPress(currentPressedLetter);
        }
        isLetterPressed = false;
        currentPressedLetter = -1;
    }

    if (numberPressed != -1 && millis() - lastNumberDebounce > debounceDelay)
    {
        lastNumberDebounce = millis();
        handleNumberPress(numberPressed);
    }

    // Check skip button
    if (digitalRead(skipPin) == LOW && millis() - lastSkipDebounce > debounceDelay)
    {
        lastSkipDebounce = millis();
        if (!continuousPlay && play && currentPlaying < queueSize + 1)
        {
            Serial.println("Skipping to next song now.");
            // mp3.stop();
            delay(500);
            if (currentPlaying < queueSize + 1)
            {
                // Reset the board to clear state
                // resetFunc();
                // playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
                Serial.print("current play: ");
                Serial.println(currentPlaying);
                // if (currentPlaying != 1)
                //  currentPlaying++;
                saveQueue();
                delay(200);
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                resetFunc();
            }
            else
            {
                queueSize = 0;
                currentPlaying = 0;
                saveQueue();
                lightAllLEDs();
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                resetFunc();
            }
        }
        else if (continuousPlay)
        {
            Serial.println("Skipping to next song in continuous mode.");
            playNextContinuous();
        }
    }

    // Update buzz/pop LEDs
    updateBuzzPopLeds();

    // Blink all LEDs if in continuous play mode
    if (continuousPlay)
    {
        if (millis() - blinkStart >= 1000)
        {
            // Serial.println("long letter pressed");
            blinkStart = millis();
            ledsOn = !ledsOn;
            for (int i = 0; i < NUM_LETTERS; i++)
                digitalWrite(letterLEDs[i], ledsOn ? HIGH : LOW);
            for (int i = 0; i < NUM_NUMBERS; i++)
                digitalWrite(numberLEDs[i], ledsOn ? HIGH : LOW);
        }
    }
    else
    {
        // Update LED display only when actively playing and within valid bounds
        if (play && currentPlaying > 0 && currentPlaying <= queueSize)
        {
            // currentPlaying is 1-indexed during playback, so subtract 1 for array access
            int playingIndex = currentPlaying - 1;
            if (playingIndex >= 0 && playingIndex < MAX_QUEUE)
            {
                showLed(queue[playingIndex].letter, queue[playingIndex].number);
            }
        }
    }

    // Check if song finished using busy pin
    if (digitalRead(busyPin) == HIGH)
    {
        // Serial.println("Song finished");
        //  Move to next song in queue
        if (donePlaying && play)
        {
            if (!continuousPlay)
            {
                Serial.println("Song done, moving to next in queue.");
                Serial.println(currentPlaying);
                if (currentPlaying == 3)
                {
                    // Queue finished
                    Serial.println("max que all  leds on");
                    queueSize = 0;
                    currentPlaying = 0;
                    saveQueue(); // Save reset values
                    lightAllLEDs();
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                    resetFunc();
                }

                if (currentPlaying < (queueSize + 1))
                {
                    Serial.print("Playing next song in queue.. ");
                    Serial.println(queue[currentPlaying].letter);
                    Serial.print("Number: ");
                    Serial.println(queue[currentPlaying].number);
                    // Reset the board to clear state
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                    delay(500);
                    resetFunc();
                    playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
                    currentPlaying++;
                    saveQueue(); // Save after incrementing currentPlaying
                    delay(500);  // brief delay to allow mp3 module to start
                }
                else
                {
                    // Queue finished
                    Serial.println("max que all  leds on");
                    queueSize = 0;
                    currentPlaying = 0;
                    saveQueue(); // Save reset values
                    lightAllLEDs();
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                    resetFunc();
                }
            }
            else
            {
                // Continuous play: play next song
                playNextContinuous();
            }
            donePlaying = false;
        }
    }
    else
    {
        donePlaying = true;
        // Serial.println("Song playing");
    }
}

void showLed(int letterIndex, int numberIndex)
{
    // Validate indices to prevent out-of-bounds access
    if (letterIndex < 0 || letterIndex >= NUM_LETTERS ||
        numberIndex < 0 || numberIndex >= NUM_NUMBERS)
    {
        Serial.print("Invalid LED indices: letter=");
        Serial.print(letterIndex);
        Serial.print(", number=");
        Serial.println(numberIndex);
        return;
    }

    // When queue is full and playing, show only the current song's LEDs
    if (queueSize == 3 && play)
    {
        oldPlay = currentPlaying;
        // Turn off all LEDs
        for (int i = 0; i < NUM_LETTERS; i++)
            digitalWrite(letterLEDs[i], LOW);
        for (int i = 0; i < NUM_NUMBERS; i++)
            digitalWrite(numberLEDs[i], LOW);

        // Turn on only the current song's LEDs (indices are already 0-based)
        digitalWrite(letterLEDs[letterIndex], HIGH);
        digitalWrite(numberLEDs[numberIndex], HIGH);

        Serial.print("LED Display: ");
        Serial.print(letters[letterIndex]);
        Serial.println(numberIndex + 1);
    }
    else
    {
        // Reset LEDs when not playing or queue not full
        for (int i = 0; i < NUM_LETTERS; i++)
            digitalWrite(letterLEDs[i], HIGH);
        for (int i = 0; i < NUM_NUMBERS; i++)
            digitalWrite(numberLEDs[i], HIGH);
    }
}

int getPressedKey(int *pins, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (digitalRead(pins[i]) == LOW)
        {
            delay(20); // basic debounce
            if (digitalRead(pins[i]) == LOW)
                return i;
        }
    }
    return -1;
}

void handleLetterPress(int index)
{
    Serial.print("Letter pressed: ");
    Serial.println(letters[index]);

    // Store selected letter
    currentLetter = index;
    digitalWrite(letterLEDs[index], HIGH); // keep LED on
    // delay(500);
}

void handleNumberPress(int index)
{
    // Only valid if a letter was pressed before
    if (currentLetter == -1)
    {
        Serial.println("Ignored: Number pressed without letter.");
        return;
    }

    int number = (index == 9) ? 0 : (index + 1);
    currentNumber = number;
    Serial.print("Number pressed: ");
    Serial.println(number);

    digitalWrite(numberLEDs[index], HIGH);

    // Add to queue
    if (queueSize < MAX_QUEUE)
    {
        queue[queueSize].letter = currentLetter;
        queue[queueSize].number = index;
        queueSize++;
        Serial.print("Queued song ");
        Serial.println(queueSize);
        saveQueue(); // Save after adding to queue

        // After 3rd selection, turn off all LEDs except the current pair
        // if (queueSize == 2 )
        // {
        //     for (int i = 0; i < NUM_LETTERS; i++)
        //         digitalWrite(letterLEDs[i], LOW);
        //     for (int i = 0; i < NUM_NUMBERS; i++)
        //         digitalWrite(numberLEDs[i], LOW);
        //     digitalWrite(letterLEDs[currentLetter], HIGH);
        //     digitalWrite(numberLEDs[index], HIGH);
        // }
    }

    // If no song is playing, start playing
    if (queueSize > 0 && currentPlaying == 0)
    {

        Serial.println("Starting playback from queue immediately after number entered.");
        playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
        play = true;
        currentPlaying = 1;
        saveQueue(); // Save currentPlaying after starting
        delay(200);  // brief delay to allow mp3 module to start
    }
    currentLetter = -1; // reset letter selection
    currentNumber = -1; // reset number selection
}

void playSong(int letterIndex, int numberIndex)
{
    // Store the currently playing song
    lastPlayedLetter = letterIndex;
    lastPlayedNumber = numberIndex;

    int number = ((numberIndex + 1) % 10);
    if (number == 0)
        number = 10;
    int trackNumber = (letterIndex * 10) + number;
    if (trackNumber > MAX_TRACKS)
        trackNumber = MAX_TRACKS;

    Serial.print("Playing track: ");
    Serial.println(trackNumber);
    Serial.print("Playing song: ");
    Serial.println(letters[letterIndex]);
    Serial.println(number);
    Serial.print("current playing: ");
    Serial.println(currentPlaying);
    // delay(1000);
    mp3.play(trackNumber);
    Serial.print("Playing song: ");
    Serial.println(letters[letterIndex]);
    Serial.println(number);
    Serial.print("current playing: ");
    Serial.println(currentPlaying);

    // Start buzz/pop sequence
    startBuzzPopSequence();

    // For the last song in queue, turn off all LEDs except this pair
    // if (currentPlaying == 2 && !continuousPlay)
    // {
    //     for (int i = 0; i < NUM_LETTERS; i++)
    //         digitalWrite(letterLEDs[i], LOW);
    //     for (int i = 0; i < NUM_NUMBERS; i++)
    //         digitalWrite(numberLEDs[i], LOW);
    //     digitalWrite(letterLEDs[letterIndex], HIGH);
    //     digitalWrite(numberLEDs[numberIndex], HIGH);
    // }
}

void lightAllLEDs()
{
    for (int i = 0; i < NUM_LETTERS; i++)
        digitalWrite(letterLEDs[i], HIGH);
    for (int i = 0; i < NUM_NUMBERS; i++)
        digitalWrite(numberLEDs[i], HIGH);
    Serial.println("All LEDs re-enabled for next round.");
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

void startContinuousPlay(int letter, int number)
{
    Serial.println("Starting continuous play from:");
    Serial.print("Letter: ");
    Serial.println(letters[letter]);
    Serial.print("Number: ");
    Serial.println(number);

    continuousPlay = true;
    startLetter = letter;
    startNumber = number;
    contLetter = letter;
    contNumber = number;
    play = true;

    // Clear the queue when starting continuous play
    queueSize = 0;
    currentPlaying = 0;

    // Initialize blink timing
    blinkStart = millis();

    // Save continuous play state to EEPROM
    EEPROM.write(EEPROM_CONTINUOUS_PLAY_ADDR, 1);
    EEPROM.write(EEPROM_START_LETTER_ADDR, startLetter);
    EEPROM.write(EEPROM_START_NUMBER_ADDR, startNumber);
    EEPROM.write(EEPROM_CONT_LETTER_ADDR, contLetter);
    EEPROM.write(EEPROM_CONT_NUMBER_ADDR, contNumber);
    EEPROM.write(EEPROM_QUEUE_SIZE_ADDR, 0);
    EEPROM.write(EEPROM_CURRENT_PLAYING_ADDR, 0);

    // Only reset if not already playing (to avoid restarting current song)
    if (!play)
    {
        // Reset to play the song
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
        delay(500);
        resetFunc();
    }
}

void playNextContinuous()
{
    // Increment number
    contNumber++;
    if (contNumber >= NUM_NUMBERS)
    {
        contNumber = 0;
        contLetter++;
        if (contLetter >= NUM_LETTERS)
        {
            contLetter = 0;
        }
    }

    Serial.println("Playing next in continuous mode:");
    Serial.print("Letter: ");
    Serial.println(letters[contLetter]);
    Serial.print("Number: ");
    Serial.println(contNumber + 1);

    // Save updated continuous play state to EEPROM
    EEPROM.write(EEPROM_CONT_LETTER_ADDR, contLetter);
    EEPROM.write(EEPROM_CONT_NUMBER_ADDR, contNumber);
    // Reset to play the next song
    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
    delay(1000);
    resetFunc();
}