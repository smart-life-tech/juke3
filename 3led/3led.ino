// Include all libraries at the very top
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <EEPROM.h>

// Configuration defines
#define NUM_LETTERS 10
#define NUM_NUMBERS 10
#define MAX_TRACKS 100
#define MAX_QUEUE 3

// Traffic Light pins (each LED has Red, Amber, Green)
// Common anode: White = +, Colors = Ground to light
#define LED1_RED A3
#define LED1_AMBER A4
#define LED1_GREEN A5
#define LED2_RED A6
#define LED2_AMBER A7
#define LED2_GREEN A8
#define LED3_RED A9
#define LED3_AMBER A10
#define LED3_GREEN A11

// Traffic Light states
enum TrafficState
{
    STATE_RED,
    STATE_AMBER,
    STATE_GREEN
};
TrafficState trafficLED1 = STATE_RED;
TrafficState trafficLED2 = STATE_RED;
TrafficState trafficLED3 = STATE_RED;
// Blinking green traffic light state
bool trafficBlinking1 = false;
bool trafficBlinking2 = false;
bool trafficBlinking3 = false;
unsigned long trafficBlinkStart = 0;
bool trafficBlinkOn = true;
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
const unsigned long beckonInterval = 20000; // 8 minutes 480
// Selection mode state
bool selectionModeEnabled = true;
int selectionCount = 0;
int pendingLetter = -1;
// Light show state
bool lightShowRunning = false;
unsigned long lightShowEnd = 0;
int lightShowIndex = 0;
unsigned long lastLightShowStep = 0;
const unsigned long lightShowStepDelay = 80;
bool pendingPlayAfterShow = false;
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
// Debug flag to reduce noisy Serial output
const bool DEBUG = false;

struct Song
{
    int letter;
    int number;
};
const int inhibitPin = 52; // Pin connected to Nayax inhibit wire
bool inhibitActive = false;
const int interruptPin = A2; // Using pin 2 for interrupt (can be changed)
int swipeCounter = 0;
int missCounter = 0;
bool swiped = false;
// Lockout state: when a swipe occurs after long inactivity, play beckon immediately
// and prevent selections until user resets the board.
bool swipeLockoutActive = false;
// Forward declarations
void playSong(int letterIndex, int numberIndex, int queueIndex = -1, bool isBeckon = false);
void setTrafficLight(int ledNum, TrafficState state);
void returnTrafficLightToRed(int queueIndex);
void startLightShow();
int getPressedKey(int *pins, int count);
void showLed(int letterIndex, int numberIndex);
void lightAllLEDs();
void startContinuousPlay(int letter, int number);
void handleLetterPress(int index);
void handleNumberPress(int index);
void playNextContinuous();
void updateBuzzPopLeds();
void startBuzzPopSequence();

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
// EEPROM address to store whether selection mode is enabled (1 = enabled, 0 = disabled)
#define EEPROM_SELECTION_MODE_ADDR 18
// EEPROM flag to request a light show on the next resumed queued song (set before reset)
#define EEPROM_LIGHTSHOW_NEXT_ADDR 19
// EEPROM address to store the queued index (0-based) for which the light show should run
#define EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR 20
// EEPROM addresses to store traffic light states (0=RED, 1=AMBER, 2=GREEN)
#define EEPROM_TRAFFIC_LED1_ADDR 21
#define EEPROM_TRAFFIC_LED2_ADDR 22
#define EEPROM_TRAFFIC_LED3_ADDR 23
// EEPROM address to persist Nayax inhibit state (1 = inhibit active)
#define EEPROM_INHIBIT_STATE_ADDR 24
// EEPROM address to persist card swipe state (1 = swiped)
#define EEPROM_SWIPED_FLAG_ADDR 25
// eeprom to save light running
#define EEPROM_LIGHTSHOW_RUNNING_ADDR 26
// buzz state
#define EEPROM_BUZZ_STATE_ADDR 27

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
    // Clear all relevant EEPROM addresses on hardware reset
    for (int i = 0; i < 28; i++)
    {
        EEPROM.write(i, 0);
    }
}

void saveTrafficLightStates()
{
    EEPROM.write(EEPROM_TRAFFIC_LED1_ADDR, trafficLED1);
    EEPROM.write(EEPROM_TRAFFIC_LED2_ADDR, trafficLED2);
    EEPROM.write(EEPROM_TRAFFIC_LED3_ADDR, trafficLED3);
    Serial.println("Traffic light states saved to EEPROM");
}

void loadTrafficLightStates()
{
    trafficLED1 = (TrafficState)EEPROM.read(EEPROM_TRAFFIC_LED1_ADDR);
    trafficLED2 = (TrafficState)EEPROM.read(EEPROM_TRAFFIC_LED2_ADDR);
    trafficLED3 = (TrafficState)EEPROM.read(EEPROM_TRAFFIC_LED3_ADDR);

    setTrafficLight(1, trafficLED1);
    setTrafficLight(2, trafficLED2);
    setTrafficLight(3, trafficLED3);

    Serial.println("Traffic light states restored from EEPROM");
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
        Serial.println("Nayax inhibit DEACTIVATED - card can be  used");
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

    // Initialize Nayax inhibit control pin
    pinMode(inhibitPin, OUTPUT);

    // Initialize Traffic Light LEDs
    pinMode(LED1_RED, OUTPUT);
    pinMode(LED1_AMBER, OUTPUT);
    pinMode(LED1_GREEN, OUTPUT);
    pinMode(LED2_RED, OUTPUT);
    pinMode(LED2_AMBER, OUTPUT);
    pinMode(LED2_GREEN, OUTPUT);
    pinMode(LED3_RED, OUTPUT);
    pinMode(LED3_AMBER, OUTPUT);
    pinMode(LED3_GREEN, OUTPUT);
    pinMode(interruptPin, INPUT);
    // Initialize inhibit pin
    pinMode(inhibitPin, OUTPUT);
    digitalWrite(inhibitPin, LOW); // Start with inhibit disabled (0v)
    Serial.println("inhibit started with disabled initially");
    // Check reset flag
    int resetFlag = EEPROM.read(EEPROM_RESET_FLAG_ADDR);
    if (resetFlag == 1)
    {
        Serial.println("Software reset detected, clearing flag.");
        // Software reset: restore traffic light states from EEPROM
        loadTrafficLightStates();
        // Clear flag and proceed normally
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
        Serial.println("Software reset detected, clearED flag.");
        if (!mp3.begin(mp3Serial, false, false))
        {
            Serial.println("DFPlayer Mini not found!, proceeding regardless, no effect");
            // while (true)
            //     ;
        }
        mp3.setTimeOut(50); // Set timeout to prevent hangs
        mp3.volume(30);     // Set volume
    }
    else
    {
        // Hardware reset: clear EEPROM and set traffic lights to RED (default state)
        clearEEPROM();
        Serial.println("Hardware reset detected, clearing EEPROM.");
        // Set all traffic lights to RED (default state)
        // Common anode: LOW = ON, HIGH = OFF
        setTrafficLight(1, STATE_RED);
        setTrafficLight(2, STATE_RED);
        setTrafficLight(3, STATE_RED);
        trafficLED1 = STATE_RED;
        trafficLED2 = STATE_RED;
        trafficLED3 = STATE_RED;

        // Ensure variables are in initial state for hardware reset
        play = false;
        queueSize = 0;
        currentPlaying = 0;
        selectionCount = 0;
        pendingLetter = -1;
        continuousPlay = false;
        beckonPlaying = false;

        delay(1000);
        if (!mp3.begin(mp3Serial, true, true))
        {
            Serial.println("DFPlayer Mini not found!, proceeding regardless, no effect");
            // while (true)
            //     ;
        }
        mp3.setTimeOut(50); // Set timeout to prevent hangs
        mp3.volume(30);     // Set volume
    }

    // Restore persisted inhibit/swipe states across software resets
    {
        int inhibitVal = EEPROM.read(EEPROM_INHIBIT_STATE_ADDR);
        int swipedVal = EEPROM.read(EEPROM_SWIPED_FLAG_ADDR);
        swiped = (swipedVal == 1);

        // Only restore inhibit if we're actually in a playing state
        // Otherwise, clear inhibit to allow new card swipes
        if (!play && queueSize == 0 && currentPlaying == 0)
        {
            Serial.println("Not in play state - clearing inhibit regardless of EEPROM value");
            setInhibit(false);
            EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 0);
        }
        else
        {
            setInhibit(inhibitVal == 1);
            Serial.print("Restored inhibit from EEPROM: ");
            Serial.println(inhibitVal == 1 ? "ACTIVE" : "INACTIVE");
        }
    }

    // Load persisted selection mode state (1 = enabled, 0 = disabled)
    int selModeVal = EEPROM.read(EEPROM_SELECTION_MODE_ADDR);
    // EEPROM value 1 indicates selection mode has been explicitly DISABLED.
    // Any other value (including 0 after a hardware reset) means selection mode defaults to enabled.
    if (selModeVal == 1)
        selectionModeEnabled = false;
    else
        selectionModeEnabled = true;

    Serial.print("Selection mode loaded: ");
    Serial.print(selectionModeEnabled ? "ENABLED" : "DISABLED");
    Serial.print(" (EEPROM value: ");
    Serial.print(selModeVal);
    Serial.println(")");

    // Check beckon flag
    int beckonFlag = EEPROM.read(EEPROM_BECKON_FLAG_ADDR);
    if (beckonFlag == 1)
    {
        // Beckon reset: play beckon song and clear flag
        int beckonLetter = EEPROM.read(EEPROM_BECKON_LETTER_ADDR);
        int beckonNumber = EEPROM.read(EEPROM_BECKON_NUMBER_ADDR);
        Serial.println("Beckon reset detected, playing beckon song.");
        // Indicate this is a beckon playback so we do NOT enable Nayax inhibit
        playSong(beckonLetter, beckonNumber, -1, true);
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
            // If a lightshow-on-next was requested before reset, run the show and start that queued song now
            int lsFlag = EEPROM.read(EEPROM_LIGHTSHOW_NEXT_ADDR);
            if (lsFlag == 1)
            {
                int lsIndex = EEPROM.read(EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR);
                Serial.print("Lightshow-on-next detected for queue index: ");
                Serial.println(lsIndex);
                // Clear the request so it doesn't run again
                EEPROM.write(EEPROM_LIGHTSHOW_NEXT_ADDR, 0);
                // Validate index
                if (lsIndex >= 0 && lsIndex < MAX_QUEUE && lsIndex < queueSize)
                {
                    // Start chaser and play the queued song immediately (they run together)
                    // Ensure we are no longer in beckon mode before starting queued playback
                    beckonPlaying = false;
                    Serial.println("Starting lightshow-on-next playback.");
                    startLightShow();
                    delay(150); // Brief delay to let lightshow begin before starting MP3
                    playSong(queue[lsIndex].letter, queue[lsIndex].number, lsIndex);

                    // Set traffic light to blinking GREEN for this song
                    int ledNum = (lsIndex % 3) + 1;
                    setTrafficLight(ledNum, STATE_GREEN);
                    trafficBlinkStart = millis();
                    trafficBlinkOn = true;
                    if (ledNum == 1)
                    {
                        trafficLED1 = STATE_GREEN;
                        trafficBlinking1 = true;
                    }
                    else if (ledNum == 2)
                    {
                        trafficLED2 = STATE_GREEN;
                        trafficBlinking2 = true;
                    }
                    else if (ledNum == 3)
                    {
                        trafficLED3 = STATE_GREEN;
                        trafficBlinking3 = true;
                    }
                    saveTrafficLightStates();

                    play = true;
                    // Set currentPlaying to 1-based count for LED display logic (playingIndex = currentPlaying - 1)
                    currentPlaying = lsIndex + 1;
                    saveQueue();
                    delay(500);
                }
            }
            if (currentPlaying > queueSize)
            {
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                Serial.println("Queue finished, resetting.");
                delay(500);
                resetFunc();
            }
            // Only resume if there's a valid queued song that hasn't been played yet
            // and we haven't already started playback via lightshow-on-next
            if (!play && (queueSize == 3 || queueSize == 2 || queueSize == 1) && currentPlaying != 0)
            {
                Serial.println("Resuming playback from EEPROM.");
                Serial.print("current playing (1-based): ");
                Serial.println(currentPlaying);
                int resumeIndex = currentPlaying - 1; // convert to 0-based for array access
                if (resumeIndex >= 0 && resumeIndex < queueSize)
                {
                    // Ensure we are no longer in beckon mode before resuming queued playback
                    beckonPlaying = false;
                    playSong(queue[resumeIndex].letter, queue[resumeIndex].number, resumeIndex);
                    play = true;
                    // currentPlaying stays at its current value - it will be incremented when song finishes
                    saveQueue(); // Save after resuming
                    delay(500);  // brief delay to allow mp3 module to start
                }
            }
        }
    }
    // queueSize = 0;
    // currentPlaying = 0;

    // Initialize activity timestamps to prevent false lockout on first swipe
    lastActivityTime = millis();
    lastBeckonTime = millis();

    Serial.println("Code 3 led Ready! v3.79");

    // Debug: print queue and playback state on startup
    Serial.print("DEBUG: queueSize=");
    Serial.print(queueSize);
    Serial.print(", currentPlaying=");
    Serial.print(currentPlaying);
    Serial.print(", play=");
    Serial.print(play);
    Serial.print(", selectionMode=");
    Serial.print(selectionModeEnabled);
    Serial.print(", continuousPlay=");
    Serial.println(continuousPlay);
    for (int i = 0; i < queueSize; i++)
    {
        Serial.print("DEBUG: queue[");
        Serial.print(i);
        Serial.print("] = ");
        Serial.print(letters[queue[i].letter]);
        Serial.print(queue[i].number + 1);
        Serial.println();
    }
    Serial.print("swipwd state = ");
    Serial.println(swiped ? "SWIPED" : "NOT SWIPED");
    Serial.print("inhibit state = ");
    Serial.println(inhibitActive ? "ACTIVE" : "INACTIVE");
    Serial.print("bekon playing = ");
    Serial.println(beckonPlaying ? "YES" : "NO");
    swiped = EEPROM.read(EEPROM_SWIPED_FLAG_ADDR) == 1;
    beckonPlaying = EEPROM.read(EEPROM_BECKON_FLAG_ADDR) == 1;
    buzzLedOn = EEPROM.read(EEPROM_BUZZ_STATE_ADDR) == 1;
    Serial.print(" song playing? = ");
    Serial.println(digitalRead(busyPin) ? "YES" : "NO");
    Serial.print("play = ");
    Serial.println(play);
    Serial.print("continuous play = ");
    Serial.println(continuousPlay);
}

void loop()
{
    while (!swiped)
    {
        updateBuzzPopLeds();
        if (digitalRead(busyPin) == LOW)
            lastBeckonTime = millis();
        // Check if it's time to play a beckon song (every 8 minutes while idle)
        if (!continuousPlay && digitalRead(busyPin) == HIGH && (millis() - lastBeckonTime >= beckonInterval) &&
            millis() - lastActivityTime >= beckonInterval)
        {
            Serial.println("Beckon: Playing beckon song after 8 minutes idle.");
            beckonIndex = EEPROM.read(EEPROM_BECKON_NUMBER_PLAYING);
            if (beckonIndex > 254)
            {
                beckonIndex = 0;
                Serial.println(" cleared beckon playlist");
                EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, 0);
            }
            int bLetter = beckonIndex / 10;
            int bNumber = beckonIndex % 10;

            // Play this beckon track via reset
            EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 1);
            EEPROM.write(EEPROM_BECKON_LETTER_ADDR, bLetter);
            EEPROM.write(EEPROM_BECKON_NUMBER_ADDR, bNumber);
            EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);

            // Advance beckon index for next time
            beckonIndex = (beckonIndex + 1) % 100;
            EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, beckonIndex);

            lastBeckonTime = millis();
            lastActivityTime = millis();
            Serial.println("Beckon: Triggering software reset to play beckon.");
            delay(1000);
            resetFunc();
        }

        int pinValue = analogRead(interruptPin);
        // if (DEBUG) {
        //     Serial.print("pin value = ");
        //     Serial.println(pinValue);
        // }
        delay(500);

        if (pinValue <= 0)
        {
            swipeCounter++;
            missCounter = 0; // reset misses since we got a valid read

            if (swipeCounter >= 1)
            {
                Serial.println("card SWIPING OCCURRED now on pin");
                if (DEBUG)
                {
                    Serial.print("pin value = ");
                    Serial.println(pinValue);
                }
                delay(500);
                swiped = true;
                EEPROM.write(EEPROM_SWIPED_FLAG_ADDR, 1);
                // disble the beckon play as no more play for beckon
                EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
                // Update inhibit state in EEPROM

                // If inactivity exceeded the beckon interval, immediately play next beckon
                bool inactivityExceeded = (millis() - lastActivityTime >= beckonInterval);
                if (inactivityExceeded && !play && !continuousPlay)
                {
                    Serial.println("Swipe after inactivity: starting next beckon immediately and locking selections.");
                    // Determine next beckon track from EEPROM_BECKON_NUMBER_PLAYING
                    int bIndex = EEPROM.read(EEPROM_BECKON_NUMBER_PLAYING);
                    if (bIndex > 254)
                    {
                        bIndex = 0;
                        EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, 0);
                    }
                    int bLetter = bIndex / 10;
                    int bNumber = bIndex % 10;
                    // Advance beckon index for next time
                    int nextIndex = (bIndex + 1) % 100;
                    EEPROM.write(EEPROM_BECKON_NUMBER_PLAYING, nextIndex);

                    // Start beckon playback now; mark as beckon so swipes are allowed
                    playSong(bLetter, bNumber, -1, true);
                    play = true;
                    beckonPlaying = true;
                    lastBeckonTime = millis();
                    // Allow selections during beckon playback; keep selection mode enabled
                    selectionModeEnabled = true;
                    swipeLockoutActive = false;
                    // Exit swipe wait loop so selection input can proceed
                    break;
                }
                else
                {
                    // Normal behavior: enable selection mode after card swipe
                    selectionModeEnabled = true;
                    selectionCount = 0;
                    pendingLetter = -1;
                    play = false;
                    // UPDATE: Refresh activity time to prevent beckon from triggering immediately after swipe
                    lastActivityTime = millis();
                    Serial.println("Selection mode ENABLED after card swipe");
                    if (DEBUG)
                    {
                        Serial.print("DEBUG: play = ");
                        Serial.print(play);
                        Serial.print(", selectionModeEnabled = ");
                        Serial.print(selectionModeEnabled);
                        Serial.print(", queueSize = ");
                        Serial.println(queueSize);
                    }
                    break;
                }
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
    if (swiped || beckonPlaying)
    {
        int letterPressed = getPressedKey(letterPins, NUM_LETTERS);
        int numberPressed = getPressedKey(numberPins, NUM_NUMBERS);

        // Debug: show button states
        if (letterPressed != -1)
        {
            Serial.print("DEBUG: Letter button pressed: ");
            Serial.print(letters[letterPressed]);
            Serial.print(", play=");
            Serial.print(play);
            Serial.print(", selectionMode=");
            Serial.println(selectionModeEnabled);
        }
        if (numberPressed != -1)
        {
            Serial.print("DEBUG: Number button pressed: ");
            Serial.print(numberPressed);
            Serial.print(", play=");
            Serial.print(play);
            Serial.print(", currentLetter=");
            Serial.println(currentLetter);
        }

        // If a light show is running, drive chaser frames and skip selection/playback updates
        lightShowRunning = (EEPROM.read(EEPROM_LIGHTSHOW_RUNNING_ADDR) == 1);
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
                Serial.println("Light show ended.");
                EEPROM.write(EEPROM_LIGHTSHOW_RUNNING_ADDR, 0);
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
                // If a playback-after-show was requested, start playback now
                if (pendingPlayAfterShow && queueSize > 0 && currentPlaying == 0)
                {
                    Serial.println("Light show finished — starting playback from queue (pending start).");
                    play = true;
                    mp3Serial.flush();
                    mp3Serial.end();
                    EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                    saveQueue();
                    pendingPlayAfterShow = false;
                    delay(200);
                    resetFunc();
                }
            }

            // while the light show runs, do not process further selection events
            return;
        }

        // Clear swiped flag now that we've exited the polling loop and entered selection handling
        // This prevents flag from persisting in EEPROM and blocking polling on next boot
        // swiped = false;
        // EEPROM.write(EEPROM_SWIPED_FLAG_ADDR, 0);

        // Handle letter press for longpress detection
        // Block button processing when a song is playing (except continuous mode longpress)
        // Block letter processing while lockout is active
        if (letterPressed != -1 && !isLetterPressed && (!play || beckonPlaying) && !swipeLockoutActive)
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
                // Longpress: allow continuous play once a single selection exists
                int seedLetter = -1;
                int seedNumber = -1;

                if (play && lastPlayedLetter != -1 && lastPlayedNumber != -1)
                {
                    // Already playing: use the last played song as the seed
                    seedLetter = lastPlayedLetter;
                    seedNumber = lastPlayedNumber;
                    Serial.print("Starting continuous play from last played: ");
                }
                else if (!play && queueSize > 0)
                {
                    // Not yet playing but we have at least one queued selection
                    seedLetter = queue[0].letter;
                    seedNumber = queue[0].number;
                    Serial.print("Starting continuous play from first queued: ");
                }

                if (seedLetter != -1 && seedNumber != -1)
                {
                    int displayNumber = ((seedNumber + 1) % 10);
                    if (displayNumber == 0)
                        displayNumber = 10;
                    Serial.print(letters[seedLetter]);
                    Serial.println(displayNumber);
                    delay(1000);

                    startContinuousPlay(seedLetter, seedNumber);
                    selectionModeEnabled = false;
                    EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 1);
                    if (pendingLetter != -1)
                    {
                        digitalWrite(letterLEDs[pendingLetter], HIGH);
                        pendingLetter = -1;
                    }
                }
                else
                {
                    Serial.println("Cannot start continuous play - no song ready");
                }
            }
            else if (pressDuration <= 1000 && millis() - lastLetterDebounce > debounceDelay && (!play || beckonPlaying) && !swipeLockoutActive)
            {
                lastLetterDebounce = millis();
                handleLetterPress(currentPressedLetter);
            }
            isLetterPressed = false;
            currentPressedLetter = -1;
        }

        // Block number processing while lockout is active
        if (numberPressed != -1 && millis() - lastNumberDebounce > debounceDelay && (!play || beckonPlaying) && !swipeLockoutActive)
        {
            lastNumberDebounce = millis();
            handleNumberPress(numberPressed);
        }

        // Check skip button
        if (digitalRead(skipPin) == LOW && millis() - lastSkipDebounce > debounceDelay && !beckonPlaying)
        {
            lastSkipDebounce = millis();
            if (!swipeLockoutActive && !continuousPlay && play && currentPlaying <= queueSize)
            {
                Serial.println("Skipping to next song now.");
                Serial.print("Current playing: ");
                Serial.println(currentPlaying);
                Serial.print("Queue size: ");
                Serial.println(queueSize);
                delay(500);

                if (currentPlaying < queueSize)
                {
                    // Return traffic light to RED for the song being skipped
                    int skippedSongIndex = currentPlaying - 1;
                    returnTrafficLightToRed(skippedSongIndex);

                    // Advance to next song and prepare via reset (required by board)
                    currentPlaying++;
                    // currentPlaying is 1-based after increment; store 0-based index for resume
                    EEPROM.write(EEPROM_LIGHTSHOW_NEXT_ADDR, 1);
                    EEPROM.write(EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR, currentPlaying - 1);
                    saveQueue();
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                    delay(200);
                    resetFunc();
                }
                else
                {
                    // Last song or queue finished - reset to selection mode
                    Serial.println("Skip: Queue finished, resetting to selection mode.");
                    queueSize = 0;
                    currentPlaying = 0;
                    play = false;
                    saveQueue();
                    EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 0);
                    // Disable Nayax inhibit and clear swipe for next round
                    setInhibit(false);
                    EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 0);
                    EEPROM.write(EEPROM_SWIPED_FLAG_ADDR, 0);
                    swiped = false;
                    lightAllLEDs();
                    EEPROM.write(EEPROM_RESET_FLAG_ADDR, 0);
                    delay(500);
                    resetFunc();
                }
            }
            else if (!swipeLockoutActive && continuousPlay)
            {
                Serial.println("Skipping to next song in continuous mode.");
                playNextContinuous();
            }
            else if (beckonPlaying)
            {
                Serial.println("Skip during beckon: ending beckon playback.");
                beckonPlaying = false;
                play = false;
                // Reset buzz/pop LED state
                buzzLedOn = false;
                popLedOn = false;
                EEPROM.write(EEPROM_BUZZ_STATE_ADDR, 0);
                digitalWrite(buzzLedPin, LOW);
                digitalWrite(popLedPin, LOW);
                // Don't call mp3.stop() - will freeze hardware. Flag will stop playback on next reset.
                lightAllLEDs();
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

        // Blink green traffic lights during play
        if (trafficBlinking1 || trafficBlinking2 || trafficBlinking3)
        {
            if (millis() - trafficBlinkStart >= 500) // 500ms blink interval
            {
                trafficBlinkStart = millis();
                trafficBlinkOn = !trafficBlinkOn;

                // Update blinking traffic lights
                if (trafficBlinking1)
                {
                    digitalWrite(LED1_GREEN, trafficBlinkOn ? LOW : HIGH);
                }
                if (trafficBlinking2)
                {
                    digitalWrite(LED2_GREEN, trafficBlinkOn ? LOW : HIGH);
                }
                if (trafficBlinking3)
                {
                    digitalWrite(LED3_GREEN, trafficBlinkOn ? LOW : HIGH);
                }
            }
        }
        else
        {
            // Update LED display only when actively playing and within valid bounds
            if (play && !beckonPlaying && currentPlaying > 0 && currentPlaying <= queueSize)
            {
                // currentPlaying is 1-indexed during playback, so subtract 1 for array access
                int playingIndex = currentPlaying - 1;
                if (playingIndex >= 0 && playingIndex < queueSize)
                {
                    showLed(queue[playingIndex].letter, queue[playingIndex].number);
                }
            }
        }

        // Continue lightshow if it's running (non-blocking animation during playback)
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
                Serial.println("Light show ended.");
                EEPROM.write(EEPROM_LIGHTSHOW_RUNNING_ADDR, 0);
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
            }
        }

        // Check if song finished using busy pin
        if (digitalRead(busyPin) == HIGH)
        {
            // Serial.println("Song finished");
            //  Move to next song in queue
            if (donePlaying && play)
            {
                if (beckonPlaying)
                {
                    // Beckon song finished - return to idle
                    Serial.println("Beckon song finished, returning to idle.");
                    beckonPlaying = false;
                    play = false;
                    // Update activity time so next beckon can trigger after 5+ min of inactivity
                    // Don't update lastBeckonTime - that's when beckon was triggered, not finished
                    lastActivityTime = millis();
                    // Reset buzz/pop LED state
                    buzzLedOn = false;
                    popLedOn = false;
                    EEPROM.write(EEPROM_BUZZ_STATE_ADDR, 0);
                    digitalWrite(buzzLedPin, LOW);
                    digitalWrite(popLedPin, LOW);
                    lightAllLEDs();
                    // resetFunc();
                }
                else if (!continuousPlay)
                {
                    Serial.println("Song done, current playing.");
                    Serial.println(currentPlaying);
                    Serial.println("Song done, moving to next in queue.");
                    Serial.println(queueSize);
                    Serial.print("DEBUG: currentPlaying=");
                    Serial.print(currentPlaying);
                    Serial.print(", queueSize=");
                    Serial.println(queueSize);
                    if (currentPlaying < queueSize)
                    {
                        Serial.print("Playing next song in queue.. ");
                        Serial.print("Index: ");
                        Serial.println(currentPlaying);

                        // Return traffic light to RED for the song that just finished
                        // currentPlaying is 1-based, so subtract 1 for 0-based queue index
                        int finishedSongIndex = currentPlaying - 1;
                        returnTrafficLightToRed(finishedSongIndex);

                        // Increment currentPlaying now so it's ready for the next song
                        currentPlaying++;
                        Serial.print("DEBUG: Incremented currentPlaying to ");
                        Serial.println(currentPlaying);
                        saveQueue();
                        // Request lightshow on next resume for the next queued index, then reset
                        EEPROM.write(EEPROM_LIGHTSHOW_NEXT_ADDR, 1);
                        // currentPlaying is 1-based after increment; store 0-based index for resume
                        EEPROM.write(EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR, currentPlaying - 1);
                        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                        Serial.println("Song done, playing next: ");
                        Serial.println(currentPlaying);
                        delay(500);
                        resetFunc();
                    }
                    else
                    {
                        // Queue finished
                        Serial.println("max que all  leds on");

                        // Return last traffic light to RED
                        int finishedSongIndex = currentPlaying - 1;
                        returnTrafficLightToRed(finishedSongIndex);

                        queueSize = 0;
                        currentPlaying = 0;
                        play = false;
                        saveQueue(); // Save reset values
                        // Re-enable selection mode on next boot
                        EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 0);
                        lightAllLEDs();

                        // Disable Nayax inhibit and clear swipe for next round
                        setInhibit(false);
                        EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 0);
                        EEPROM.write(EEPROM_SWIPED_FLAG_ADDR, 0);
                        swiped = false;

                        // Reset all traffic lights to RED for next round
                        setTrafficLight(1, STATE_RED);
                        setTrafficLight(2, STATE_RED);
                        setTrafficLight(3, STATE_RED);
                        trafficLED1 = STATE_RED;
                        trafficLED2 = STATE_RED;
                        trafficLED3 = STATE_RED;

                        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                        delay(500);
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

    // When playing queued songs (not beckon), show only the current song's LEDs
    if (play && !beckonPlaying && currentPlaying > 0 && currentPlaying <= queueSize)
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
        // Reset LEDs when not playing
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

    // Traffic Light: Change corresponding LED from RED to AMBER based on queue position
    // Use modulo 3 to map any letter to one of the 3 traffic lights
    int ledNum = (queueSize % 3) + 1; // Maps to 1, 2, or 3
    setTrafficLight(ledNum, STATE_AMBER);

    if (ledNum == 1)
    {
        trafficLED1 = STATE_AMBER;
    }
    else if (ledNum == 2)
    {
        trafficLED2 = STATE_AMBER;
    }
    else if (ledNum == 3)
    {
        trafficLED3 = STATE_AMBER;
    }
    saveTrafficLightStates();

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

    // Traffic Light: Change corresponding LED from AMBER to GREEN
    // Use modulo 3 to map to one of the 3 traffic lights based on queue position
    int ledNum = (queueSize % 3) + 1; // Maps to 1, 2, or 3
    setTrafficLight(ledNum, STATE_GREEN);

    if (ledNum == 1)
    {
        trafficLED1 = STATE_GREEN;
    }
    else if (ledNum == 2)
    {
        trafficLED2 = STATE_GREEN;
    }
    else if (ledNum == 3)
    {
        trafficLED3 = STATE_GREEN;
    }
    saveTrafficLightStates();

    // Add to queue
    if (queueSize < MAX_QUEUE)
    {
        queue[queueSize].letter = currentLetter;
        queue[queueSize].number = index;
        queueSize++;
        selectionCount = queueSize; // track confirmed selections
        Serial.print("Queued song ");
        Serial.println(queueSize);
        Serial.print("current playing: ");
        Serial.println(currentPlaying);
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
            // Persist selection mode disabled so it survives reset cycles between songs
            EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 1);
            Serial.println("Selection mode complete (3 selections). Starting light show and starting playback.");
            // If a beckon was playing, stop it now and clear beckon state
            if (beckonPlaying)
            {
                Serial.println("Stopping beckon playback to start queued songs.");
                beckonPlaying = false;
                EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
                // mp3.stop();
            }
            // Clear beckonPlaying before reset so queued songs play normally
            beckonPlaying = false;
            // Start the light show (non-blocking)
            startLightShow();
            // Board requires reset before playback to avoid freeze: request lightshow+play on next boot
            if (queueSize > 0 && currentPlaying == 0)
            {
                EEPROM.write(EEPROM_LIGHTSHOW_NEXT_ADDR, 1);
                EEPROM.write(EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR, 0);
                EEPROM.write(EEPROM_CURRENT_PLAYING_ADDR, 0); // Explicitly set currentPlaying to 0
                saveQueue();
                EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
                delay(200);
                resetFunc();
            }
        }
    }

    // If not in selection mode, and no song is playing, start playing immediately
    // NOTE: This path is for non-selection-mode queueing (e.g., individual song selections after selection mode)
    // Do not execute if we just finished selection mode (handled above with resetFunc at line 762)
    if (!selectionModeEnabled && queueSize >= 3 && currentPlaying == 0)
    {
        Serial.println("Starting playback from queue after selection.");
        // If a beckon was playing, stop it now
        if (beckonPlaying)
        {
            Serial.println("Stopping beckon playback to start queued songs.");
            beckonPlaying = false;
            // mp3.stop();
        }
        play = true;
        mp3Serial.flush();
        mp3Serial.end();
        EEPROM.write(EEPROM_BECKON_FLAG_ADDR, 0);
        EEPROM.write(EEPROM_RESET_FLAG_ADDR, 1);
        EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 1);
        EEPROM.write(EEPROM_LIGHTSHOW_QUEUE_INDEX_ADDR, 0);
        EEPROM.write(EEPROM_LIGHTSHOW_NEXT_ADDR, 1); // lightshow needed for single selection
        saveQueue();                                 // Save currentPlaying after starting
        delay(500);                                  // brief delay to allow mp3 module to start
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
    EEPROM.write(EEPROM_LIGHTSHOW_RUNNING_ADDR, 1);
    lightShowEnd = millis() + 7000;
    lightShowIndex = 0;
    lastLightShowStep = 0;
    // ensure all LEDs are off to start
    for (int i = 0; i < NUM_LETTERS + NUM_NUMBERS; i++)
        digitalWrite(allLEDs[i], LOW);
}

void playSong(int letterIndex, int numberIndex, int queueIndex = -1, bool isBeckon = false)
{
    // Debug: print when playSong is called with index information
    Serial.print("DEBUG playSong called. queueIndex=");
    Serial.print(queueIndex);
    Serial.print(", currentPlaying=");
    Serial.println(currentPlaying);
    // For normal playback activate Nayax inhibit so card/swipes are blocked while playing.
    // For beckon playback, keep inhibit disabled so swipes are allowed during the beckon song.
    if (!isBeckon)
    {
        setInhibit(true);
        EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 1);
        Serial.println("Beckon playback: Nayax inhibit ENABLED to dis-allow swipes");
    }
    else
    {
        // Ensure inhibit is disabled for beckon playback to allow swipes
        setInhibit(false);
        EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 0);
        Serial.println("Beckon playback: Nayax inhibit left DISABLED to allow swipes");
    }
    // Determine whether to trigger the 7s light show for this playback.
    // If `queueIndex` is provided (0-based index into the queued songs), trigger show
    // for the 2nd or 3rd queued song (indices 1 and 2).
    if (!selectionModeEnabled && !lightShowRunning && queueIndex >= 0)
    {
        if (queueIndex == 1 || queueIndex == 2)
        {
            Serial.print("Triggering light show for queued song index: ");
            Serial.println(queueIndex);
            startLightShow();
        }
    }

    // Traffic Light: Change LED to blinking GREEN when song starts playing
    if (queueIndex >= 0 && queueIndex < MAX_QUEUE)
    {
        int ledNum = (queueIndex % 3) + 1; // Maps to 1, 2, or 3
        setTrafficLight(ledNum, STATE_GREEN);

        // Set blinking flags
        trafficBlinkStart = millis();
        trafficBlinkOn = true;
        if (ledNum == 1)
        {
            trafficLED1 = STATE_GREEN;
            trafficBlinking1 = true;
        }
        else if (ledNum == 2)
        {
            trafficLED2 = STATE_GREEN;
            trafficBlinking2 = true;
        }
        else if (ledNum == 3)
        {
            trafficLED3 = STATE_GREEN;
            trafficBlinking3 = true;
        }
        saveTrafficLightStates();

        Serial.print("Song playing: LED ");
        Serial.print(ledNum);
        Serial.println(" changed to blinking GREEN");
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
    Serial.print(letters[letterIndex]);
    Serial.println(number);
    Serial.print("current playing: ");
    Serial.println(currentPlaying);
    // delay(1000);
    mp3.play(trackNumber);
    Serial.print("Playing song: ");
    Serial.print(letters[letterIndex]);
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
    EEPROM.write(EEPROM_BUZZ_STATE_ADDR, 1);
    buzzStartTime = millis();
    digitalWrite(buzzLedPin, HIGH);
}

void updateBuzzPopLeds()
{
    if (buzzLedOn && millis() - buzzStartTime >= 7000)
    {
        buzzLedOn = false;
        EEPROM.write(EEPROM_BUZZ_STATE_ADDR, 0);
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
    bool wasPlaying = play;

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

    // Exit selection mode as soon as continuous play is requested
    selectionModeEnabled = false;
    EEPROM.write(EEPROM_SELECTION_MODE_ADDR, 1);
    if (pendingLetter != -1)
    {
        digitalWrite(letterLEDs[pendingLetter], HIGH);
        pendingLetter = -1;
    }
    selectionCount = 0;

    // Activate Nayax inhibit and persist before reset so it survives
    setInhibit(true);
    EEPROM.write(EEPROM_INHIBIT_STATE_ADDR, 1);

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

    // Only reset if we were not already playing (to kick off playback immediately)
    if (!wasPlaying)
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

// Function to set traffic light LED state
void setTrafficLight(int ledNum, TrafficState state)
{
    // Common anode: LOW = ON, HIGH = OFF
    int redPin, amberPin, greenPin;

    if (ledNum == 1)
    {
        redPin = LED1_RED;
        amberPin = LED1_AMBER;
        greenPin = LED1_GREEN;
    }
    else if (ledNum == 2)
    {
        redPin = LED2_RED;
        amberPin = LED2_AMBER;
        greenPin = LED2_GREEN;
    }
    else if (ledNum == 3)
    {
        redPin = LED3_RED;
        amberPin = LED3_AMBER;
        greenPin = LED3_GREEN;
    }
    else
    {
        return; // Invalid LED number
    }

    // Turn all colors OFF first
    digitalWrite(redPin, HIGH);
    digitalWrite(amberPin, HIGH);
    digitalWrite(greenPin, HIGH);

    // Turn ON the desired color
    switch (state)
    {
    case STATE_RED:
        digitalWrite(redPin, LOW);
        Serial.print("Traffic LED ");
        Serial.print(ledNum);
        Serial.println(" = RED");
        break;
    case STATE_AMBER:
        digitalWrite(amberPin, LOW);
        Serial.print("Traffic LED ");
        Serial.print(ledNum);
        Serial.println(" = AMBER");
        break;
    case STATE_GREEN:
        digitalWrite(greenPin, LOW);
        Serial.print("Traffic LED ");
        Serial.print(ledNum);
        Serial.println(" = GREEN");
        break;
    }
}

// Function to return traffic light to RED after song finishes
void returnTrafficLightToRed(int queueIndex)
{
    // Determine which traffic light to reset based on queue position
    // Use modulo 3 to map to one of the 3 traffic lights
    Serial.print("DEBUG returnTrafficLightToRed called with queueIndex=");
    Serial.println(queueIndex);

    if (queueIndex >= 0 && queueIndex < MAX_QUEUE)
    {
        int letterIndex = queue[queueIndex].letter;
        int numberIndex = queue[queueIndex].number;

        Serial.print("DEBUG letter=");
        Serial.print(letters[letterIndex]);
        Serial.print(" (index ");
        Serial.print(letterIndex);
        Serial.print("), number=");
        Serial.print(numberIndex + 1);
        Serial.print(" (index ");
        Serial.print(numberIndex);
        Serial.println(")");

        // Map queue position to traffic light using modulo 3
        int ledNum = (queueIndex % 3) + 1; // Maps to 1, 2, or 3

        Serial.print("DEBUG Returning LED ");
        Serial.print(ledNum);
        Serial.println(" to RED");

        setTrafficLight(ledNum, STATE_RED);

        if (ledNum == 1)
        {
            trafficLED1 = STATE_RED;
            trafficBlinking1 = false;
        }
        else if (ledNum == 2)
        {
            trafficLED2 = STATE_RED;
            trafficBlinking2 = false;
        }
        else if (ledNum == 3)
        {
            trafficLED3 = STATE_RED;
            trafficBlinking3 = false;
        }
        saveTrafficLightStates();
    }
    else
    {
        Serial.println("DEBUG Invalid queueIndex");
    }
}
