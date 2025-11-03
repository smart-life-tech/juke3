# TODO for EEPROM Queue Persistence in codeAK.ino

1. Add #include <EEPROM.h> at the top of the file. ✓
2. Define EEPROM addresses: queue at 0 (6 bytes for 3 Songs), queueSize at 6 (1 byte), currentPlaying at 7 (1 byte). ✓
3. Create saveQueue() function to write queue, queueSize, and currentPlaying to EEPROM. ✓
4. Create loadQueue() function to read queue, queueSize, and currentPlaying from EEPROM. ✓
5. In handleNumberPress(), after adding to queue, call saveQueue(). ✓
6. In loop(), after incrementing currentPlaying when song finishes, call saveQueue(). ✓
7. In setup(), after initialization, call loadQueue(); if queueSize == 3 and currentPlaying < queueSize, play the next song and set play=true (skip LED logic for resume). ✓
8. When queue finishes, reset queueSize=0, currentPlaying=0, call saveQueue(), and lightAllLEDs(). ✓
