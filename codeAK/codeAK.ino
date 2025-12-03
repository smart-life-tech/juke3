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
unsigned long busyHighStart = 0;
bool busyWasLow = true;
unsigned long lastBeckonTime = 0;
unsigned long lastActivityTime = 0;
int beckonIndex = 0;
bool beckonPlaying = false;
const unsigned long beckonInterval = 480000; // 8 minutes in ms
// Selection mode state
bool selectionModeEnabled = true; // true = user is in selection mode to pick up to 3 songs
int selectionCount = 0;          // confirmed selections so far (0..3)
int pendingLetter = -1;         // letter index currently selected (LED off) until number confirmed
// Light show state
bool lightShowRunning = false;
unsigned long lightShowEnd = 0;
int lightShowIndex = 0;
unsigned long lastLightShowStep = 0;
const unsigned long lightShowStepDelay = 80; // ms between chaser steps
// Letter button pins A–K (skipping I)
int letterPins[NUM_LETTERS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
// Number button pins 0–9
int numberPins[NUM_NUMBERS] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
// LED pins for letters and numbers
int letterLEDs[NUM_LETTERS] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
int numberLEDs[NUM_NUMBERS] = {42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
int allLEDs[NUM_LETTERS + NUM_NUMBERS];

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
#define EEPROM_BECKON_FLAG_ADDR 14
#define EEPROM_BECKON_LETTER_ADDR 15
#define EEPROM_BECKON_NUMBER_ADDR 16
#define EEPROM_BECKON_NUMBER_PLAYING 17

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
    Serial.println("Saved queue to EEPROM:");
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
    for (int i = 0; i < 18; i++)
    {
        EEPROM.write(i, 0);
    }
}

void setup()
{
    Serial.println("code started");
    Serial.begin(9600);
    mp3Serial.begin(9600);

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

    // build flat LED array for the 20-LED chaser: letters then numbers
    for (int i = 0; i < NUM_LETTERS; i++)
        allLEDs[i] = letterLEDs[i];
    for (int i = 0; i < NUM_NUMBERS; i++)
        allLEDs[NUM_LETTERS + i] = numberLEDs[i];

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
        Serial.println("Software reset detected, clearing flag.");
        // Software reset: clear flag and proceed normally
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
        Serial.println("Software reset detected, clearED flag.");
        if (!mp3.begin(mp3Serial, false, false))
        {
            Serial.println("DFPlayer Mini not found!, proceeding regardless, no effect");
            // while (true)
            //     ;
            // mp3.play(1);
        }
        mp3.setTimeOut(50); // Set timeout to prevent hangs
        mp3.volume(30);     // Set volume
    }
    else
    {
        // Hardware reset: clear EEPROM
        clearEEPROM();
        Serial.println("Hardware reset detected, clearing EEPROM.");
        if (!mp3.begin(mp3Serial, true, true))
        {
            Serial.println("DFPlayer Mini not found!, proceeding regardless, no effect");
            // while (true)
            //     ;
            // mp3.play(1);
        }
        mp3.setTimeOut(50); // Set timeout to prevent hangs
        mp3.volume(30);     // Set volume
    }

    // Check beckon flag
    int beckonFlag = EEPROM.read(EEPROM_BECKON_FLAG_ADDR);
    if (beckonFlag == 1)
    {
        // Beckon reset: play beckon song and clear flag
        int beckonLetter = EEPROM.read(EEPROM_BECKON_LETTER_ADDR);
        int beckonNumber = EEPROM.read(EEPROM_BECKON_NUMBER_ADDR);
        Serial.println("Beckon reset detected, playing beckon song.");
        playSong(beckonLetter, beckonNumber);
        beckonPlaying = true;
        play = true;
        EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
        delay(500);
    }
    else
    {
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
            if (currentPlaying > queueSize)
            {
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                Serial.println("Queue finished, resetting.");
                delay(500);
                resetFunc();
            }
            if (queueSize == 3 || queueSize == 2 || queueSize == 1 && currentPlaying < queueSize)
            {
                Serial.println("Resuming playback from EEPROM.");
                playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
                play = true;
                currentPlaying++;
                saveQueue(); // Save after resuming
                delay(500);  // brief delay to allow mp3 module to start
            }
        }
    }
    // queueSize = 0;
    // currentPlaying = 0;

    Serial.println("Code AK Ready! v2.29");
}

void loop()
{
    int letterPressed = getPressedKey(letterPins, NUM_LETTERS);
    int numberPressed = getPressedKey(numberPins, NUM_NUMBERS);

    // If a light show is running, drive chaser frames and skip selection/playback updates
    if (lightShowRunning)
    {
        unsigned long now = millis();
        if (now - lastLightShowStep >= lightShowStepDelay)
        {
            // turn all LEDs off
            for (int i = 0; i < NUM_LETTERS + NUM_NUMBERS; i++)
                digitalWrite(allLEDs[i], LOW);

            // light the current index
            digitalWrite(allLEDs[lightShowIndex], HIGH);

            lightShowIndex = (lightShowIndex + 1) % (NUM_LETTERS + NUM_NUMBERS);
            lastLightShowStep = now;
        }

        // end the show when time's up
        if (millis() >= lightShowEnd)
        {
            lightShowRunning = false;
            // restore LEDs for current song or default
            if (play && currentPlaying > 0 && currentPlaying <= queueSize)
            {
                int playingIndex = currentPlaying - 1;
                showLed(queue[playingIndex].letter, queue[playingIndex].number);
            }
            else if (lastPlayedLetter != -1 && lastPlayedNumber != -1)
            {
                showLed(lastPlayedLetter, lastPlayedNumber);
            }
            else
            {
                lightAllLEDs();
            }
            // If selection mode finished and there is a queued playlist ready, start playback now
            if (!selectionModeEnabled && queueSize > 0 && currentPlaying == 0)
            {
                Serial.println("Light show finished — starting playback from queue.");
                play = true;
                mp3Serial.flush();
                mp3Serial.end();
                EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                saveQueue();
                delay(200);
                resetFunc();
            }
        }

        // while the light show runs, do not process further selection events
        return;
    }

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
            Serial.print("Current playing: ");
            Serial.println(currentPlaying);
            Serial.print("Queue size: ");
            Serial.println(queueSize);
            // mp3.stop();
            delay(500);
            if (currentPlaying >= 3)
            {
                // Queue finished
                Serial.println("max que all  leds on");
                queueSize = 0;
                currentPlaying = 0;
                saveQueue(); // Save reset values
                lightAllLEDs();
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                delay(1000);
                resetFunc();
            }
            if (currentPlaying >= queueSize)
            {
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                Serial.println("Queue finished, resetting.");
                delay(1000);
                resetFunc();
            }
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
                Serial.println("Song done, current playing.");
                Serial.println(currentPlaying);
                Serial.println("Song done, moving to next in queue.");
                Serial.println(queueSize);
                if (currentPlaying == 3)
                {
                    // Queue finished
                    Serial.println("max que all  leds on");
                    queueSize = 0;
                    currentPlaying = 0;
                    saveQueue(); // Save reset values
                    lightAllLEDs();
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                    delay(1000);
                    resetFunc();
                }
                if (currentPlaying > queueSize)
                {
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                    Serial.println("Queue finished, resetting.");
                    delay(1000);
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

    // Watchdog for busyPin stuck HIGH
    if (digitalRead(busyPin) == HIGH && currentPlaying >= queueSize && play)
    {
        if (busyWasLow)
        {
            busyHighStart = millis();
            busyWasLow = false;
        }
        else if (millis() - busyHighStart >= 30000)
        {
            Serial.println("Busy pin stuck HIGH for 30 seconds, resetting board.");
            EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
            delay(500);
            resetFunc();
        }
    }
    else if (digitalRead(busyPin) == LOW)
    {
        busyWasLow = true;
    }

    // Beckon function: play a song every 8 minutes if no activity and not playing
    if (!play && !continuousPlay && digitalRead(busyPin) == HIGH && millis() - lastBeckonTime >= beckonInterval && millis() - lastActivityTime >= beckonInterval)
    {
        Serial.println("Beckon: Saving beckon song to EEPROM and resetting.");
        beckonIndex = EEPROM.read(EEPROM_BECKON_NUMBER_PLAYING);
        Serial.print("beckon index = ");
        Serial.println(beckonIndex);
        if (beckonIndex > 254)
            EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, 0);
        int beckonLetter = beckonIndex / 10;
        int beckonNumber = beckonIndex % 10;
        EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 1);
        EEPROM.write(EEPROM_BECKON_LETTER_ADDR, beckonLetter);
        EEPROM.write(EEPROM_BECKON_NUMBER_ADDR, beckonNumber);
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
        lastBeckonTime = millis();
        beckonIndex = (beckonIndex + 1) % 100; // Cycle through all 100 songs
        Serial.print("beckon index now = ");
        Serial.println(beckonIndex);
        EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, beckonIndex);
        delay(500);
        resetFunc();
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

    // Update activity time to override beckon
    lastActivityTime = millis();

    // Selection-mode behavior: toggle pending letter LED off to indicate pending selection
    if (selectionModeEnabled)
    {
        // If there was a previous pending letter, re-enable its LED
        if (pendingLetter != -1 && pendingLetter != index)
        {
            digitalWrite(letterLEDs[pendingLetter], HIGH);
        }

        // Set new pending letter and turn its LED off until number confirmed
        pendingLetter = index;
        currentLetter = index;
        digitalWrite(letterLEDs[index], LOW); // show as selected/pending
        Serial.print("Pending letter set: ");
        Serial.println(letters[index]);
        return;
    }

    // Normal (non-selection-mode) behavior: store selected letter and leave LED ON
    currentLetter = index;
    digitalWrite(letterLEDs[index], HIGH);
}

void handleNumberPress(int index)
{
    // Only valid if a letter was pressed before
    if (currentLetter == -1)
    {
        Serial.println("Ignored: Number pressed without letter.");
        return;
    }

    // Update activity time to override beckon
    lastActivityTime = millis();

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
        selectionCount = queueSize; // track confirmed selections
        Serial.print("Queued song ");
        Serial.println(queueSize);
        saveQueue(); // Save after adding to queue

        // If there was a pending letter, re-enable its LED now that number chosen
        if (pendingLetter != -1)
        {
            digitalWrite(letterLEDs[pendingLetter], HIGH);
            pendingLetter = -1;
        }

        // If selection mode and we have reached 3 selections, finish selection mode
        if (selectionModeEnabled && selectionCount >= 3)
        {
            selectionModeEnabled = false;
            Serial.println("Selection mode complete (3 selections). Starting light show.");
            startLightShow();

            // After light show completes, allow playback to start by triggering reset as before
            // We will set flags now; the actual reset will occur after show ends because the
            // chaser logic returns early during the show. When the show finishes the loop will
            // resume and playback logic (queueSize>0 && currentPlaying==0) will trigger normally.
            EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
            EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
            saveQueue();
        }
    }

    // If not in selection mode, and no song is playing, start playing immediately
    if (!selectionModeEnabled && queueSize > 0 && currentPlaying == 0)
    {
        Serial.println("Starting playback from queue after selection.");
        play = true;
        mp3Serial.flush();
        mp3Serial.end();
        EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
        saveQueue(); // Save currentPlaying after starting
        delay(500);  // brief delay to allow mp3 module to start
        resetFunc();
    }

    currentLetter = -1; // reset letter selection
    currentNumber = -1; // reset number selection
}

void startLightShow()
{
    // Initialize non-blocking light show (20-LED chaser) for 7 seconds
    Serial.println("Starting 7s light show (20-LED chaser)");
    lightShowRunning = true;
    lightShowEnd = millis() + 7000;
    lightShowIndex = 0;
    lastLightShowStep = 0;
    // ensure all LEDs are off to start
    for (int i = 0; i < NUM_LETTERS + NUM_NUMBERS; i++)
        digitalWrite(allLEDs[i], LOW);
}

void playSong(int letterIndex, int numberIndex)
{
    // If this play corresponds to the 2nd or 3rd queued song, trigger the 7s light show
    if (!selectionModeEnabled && queueSize > 0)
    {
        for (int i = 0; i < queueSize; i++)
        {
            if (queue[i].letter == letterIndex && queue[i].number == numberIndex)
            {
                if ((i == 1 || i == 2) && !lightShowRunning)
                {
                    Serial.println("Triggering light show for start of 2nd/3rd song.");
                    startLightShow();
                    // let the show run; playback will continue regardless (this call is non-blocking)
                }
                break;
            }
        }
    }
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
