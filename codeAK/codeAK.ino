#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define NUM_LETTERS 10
#define NUM_NUMBERS 10
#define MAX_TRACKS 100
#define MAX_QUEUE 3
int currentLetter = -1;
int currentNumber = -1;
const int busyPin = 12;

// Letter button pins A–K (skipping I)
int letterPins[NUM_LETTERS] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
// Number button pins 0–9
int numberPins[NUM_NUMBERS] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
// LED pins for letters and numbers
int letterLEDs[NUM_LETTERS] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41};
int numberLEDs[NUM_NUMBERS] = {42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

SoftwareSerial mp3Serial(16, 17); // RX, TX
DFRobotDFPlayerMini mp3;

char letters[NUM_LETTERS] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K'};

struct Song
{
    int letter;
    int number;
};

Song queue[MAX_QUEUE];
int queueSize = 0;
int currentPlaying = -1;

unsigned long lastLetterDebounce = 0;
unsigned long lastNumberDebounce = 0;
const unsigned long debounceDelay = 200;

void setup()
{
    Serial.begin(9600);
    mp3Serial.begin(9600);
    if (!mp3.begin(mp3Serial))
    {
        Serial.println("DFPlayer Mini not found!, proceeding regardless");
        // while (true)
        //     ;
        // mp3.play(1);
    }

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

    Serial.println("Code AK Ready!");
}

void loop()
{
    int letterPressed = getPressedKey(letterPins, NUM_LETTERS);
    int numberPressed = getPressedKey(numberPins, NUM_NUMBERS);

    if (letterPressed != -1 && millis() - lastLetterDebounce > debounceDelay)
    {
        lastLetterDebounce = millis();
        handleLetterPress(letterPressed);
    }

    if (numberPressed != -1 && millis() - lastNumberDebounce > debounceDelay)
    {
        lastNumberDebounce = millis();
        handleNumberPress(numberPressed);
    }

    // Check if song finished using busy pin
    if (digitalRead(busyPin) == HIGH)
    {
        Serial.println("Song finished");
        // Move to next song in queue
        if (currentPlaying != -1)
        {
            currentPlaying++;
            if (currentPlaying < queueSize)
            {
                playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
            }
            else
            {
                // Queue finished
                currentPlaying = -1;
                queueSize = 0;
                lightAllLEDs();
            }
        }
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
    }

    // If no song is playing, start playing
    if (currentPlaying == -1 && queueSize > 0)
    {
        currentPlaying = 0;
        playSong(queue[currentPlaying].letter, queue[currentPlaying].number);
    }
    currentLetter = -1; // reset letter selection
    currentNumber = -1; // reset number selection
}

void playSong(int letterIndex, int numberIndex)
{
    int number = ((numberIndex + 1) % 10);
    if (number == 0)
        number = 10;
    int trackNumber = (letterIndex * 10) + number;
    if (trackNumber > MAX_TRACKS)
        trackNumber = MAX_TRACKS;

    Serial.print("Playing track ");
    Serial.println(trackNumber);
    mp3.play(trackNumber);

    // For the last song in queue, turn off all LEDs except this pair
    if (currentPlaying == queueSize - 1)
    {
        for (int i = 0; i < NUM_LETTERS; i++)
            digitalWrite(letterLEDs[i], LOW);
        for (int i = 0; i < NUM_NUMBERS; i++)
            digitalWrite(numberLEDs[i], LOW);
        digitalWrite(letterLEDs[letterIndex], HIGH);
        digitalWrite(numberLEDs[numberIndex], HIGH);
    }
}

void lightAllLEDs()
{
    for (int i = 0; i < NUM_LETTERS; i++)
        digitalWrite(letterLEDs[i], HIGH);
    for (int i = 0; i < NUM_NUMBERS; i++)
        digitalWrite(numberLEDs[i], HIGH);
    // Serial.println("All LEDs re-enabled for next round.");
}
