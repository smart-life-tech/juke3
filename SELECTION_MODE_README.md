# Jukebox v3 - Selection Mode & Light Show

## Overview

The updated `codeAK.ino` introduces a **two-part LED process** that enhances the user interaction experience:

### Part 1: Selection Confirmation Mode
Users select up to 3 songs via letter-number pairs. Each selection is confirmed visually by toggling LEDs:
- When a **letter is pressed**, that letter's LED turns **OFF** (pending state), signaling a choice awaits confirmation.
- When a **number is pressed**, the letter LED turns back **ON** and the pair is queued.
- This process repeats up to **3 times** to fill the queue.
- **Light show is disabled during selection** to avoid distracting the user.

### Part 2: Light Show (20-LED Chaser)
After selections are complete, a **7-second LED chaser** runs across all 20 LEDs (10 letter + 10 number), creating a visual "celebration" before playback begins.

The light show also triggers:
- **B)** At the start of the 2nd queued song  
- **C)** At the start of the 3rd queued song  

After each 7-second show, the LEDs return to displaying the current song playing.

---

## Selection Mode Usage

### Starting State
- **All 20 LEDs are ON** (default, ready for selection).

### Selection Process (Step-by-Step)

1. **Press a Letter** (e.g., `A`)  
   - The letter LED turns **OFF** (pending confirmation).  
   - Example: LED `A` is now dark.

2. **Press a Number** (e.g., `1`)  
   - The letter LED (`A`) turns back **ON**.  
   - The number LED (`1`) is lit.  
   - The song `A1` is now **queued** (Selection #1 complete).

3. **Repeat Steps 1–2** for the second song.  
   - Once the 2nd number is pressed, that pair joins the queue.  
   - Selection #2 complete.

4. **Repeat Steps 1–2** for the third song.  
   - Once the 3rd number is pressed, Selection Mode exits automatically.  
   - The **7-second light show** begins (all LEDs cycle in a chaser pattern).

### After Selection Complete

- The **chaser runs for 7 seconds**, cycling through all 20 LED positions.  
- When the show ends, LEDs display the **currently playing song** (the first queued pair lights up).  
- **Playback starts automatically**, playing the 3 queued songs in order.

---

## Light Show Behavior

### When the Show Triggers

1. **After 3rd Selection Confirmed**  
   - The 7-second chaser runs, then playback begins.

2. **At Song 2 Start**  
   - While Song 1 is finishing and Song 2 begins, the chaser runs.  
   - After the show, Song 2's LEDs light up normally.  
   - Playback continues uninterrupted (show is non-blocking).

3. **At Song 3 Start**  
   - Same behavior as Song 2 — the chaser shows, Song 3's LEDs then display.

### Chaser Pattern

- **LED Animation:** A single LED moves sequentially through all 20 positions.  
- **Speed:** Each step is ~80ms apart, completing a full cycle in ~1.6 seconds.  
- **Duration:** Show runs for exactly 7 seconds, then stops.

---

## Test Steps

### Test 1: Basic Selection & LED Toggle
1. Power on the device — **all 20 LEDs should be ON**.
2. Press letter `A` → **LED `A` should turn OFF** (pending).
3. Press number `1` → **LED `A` should turn back ON**, and **LED `1` should stay ON**.
4. Verify **LED pair `A1` is lit** and the 1st selection is confirmed.

### Test 2: Multiple Selections
1. Starting from the end of Test 1, press letter `B` → **LED `B` turns OFF**.
2. Press number `2` → **LED `B` turns ON**, and **LED `2` lights**.
3. Verify **LEDs `A1` and `B2` are now ON**, and 2 selections are queued.
4. Press letter `C` → **LED `C` turns OFF**.
5. Press number `3` → **LED `C` turns ON**, and **LED `3` lights**.

### Test 3: Light Show After 3rd Selection
1. Complete Test 2 (3 selections made).
2. **The 7-second light show should start automatically**, cycling through all 20 LEDs rapidly.
3. After ~7 seconds, **the chaser stops**.
4. **LED pair `A1` (1st song) should light up** and remain on as playback begins.
5. Verify **music plays** for the 1st queued song (track `A1`).

### Test 4: Light Show at Song 2 Start
1. Allow the 1st song to finish naturally (or skip to Song 2 manually via skip button).
2. **The 7-second chaser should trigger as Song 2 starts**.
3. After the show, **LEDs show the current song** (e.g., `B2`).
4. Verify **music continues** without interruption.

### Test 5: Light Show at Song 3 Start
1. Allow Song 2 to finish or skip to Song 3.
2. **The 7-second chaser should trigger again as Song 3 starts**.
3. After the show, **LEDs display Song 3** (e.g., `C3`).
4. Verify playback continues normally.

### Test 6: Ensure Show Does Not Run During Selection
1. Power on — all LEDs ON.
2. Press letter `A` (LED `A` OFF) — **no chaser should start**.
3. Press number `1` (LED `A` ON) — **no chaser should start**.
4. Verify that the chaser **only runs after all 3 selections are made**, not during the selection process.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|--------------|-----|
| LED stays ON after letter press | Selection mode disabled or not entered | Restart device; check `selectionModeEnabled` default in code. |
| Light show runs too fast or too slow | `lightShowStepDelay` value adjusted | Modify `lightShowStepDelay` (default 80ms) to speed up or slow down the chaser. |
| Light show doesn't start after 3 selections | Reset/EEPROM flag not set correctly | Verify `EEPROM.write()` and `resetFunc()` are called. Check device reset behavior. |
| Playback doesn't start after selection | Selection mode not exiting properly | Ensure `queueSize == 3` and `currentPlaying == 0` before light show ends. |

---

## Configuration & Customization

### Chaser Speed
To change the speed of the LED chaser:
- Locate `const unsigned long lightShowStepDelay = 80;` in `codeAK.ino`.
- **Lower values** (e.g., 50ms) = faster chaser.  
- **Higher values** (e.g., 150ms) = slower chaser.

### Light Show Duration
To change how long the show runs:
- Locate `lightShowEnd = millis() + 7000;` in the `startLightShow()` function.
- Replace `7000` with the desired milliseconds (e.g., `5000` for 5 seconds, `10000` for 10 seconds).

### Disable Selection Mode (Always Play Immediately)
To skip selection mode and allow immediate playback on number press:
- Find `bool selectionModeEnabled = true;` near the top of `codeAK.ino`.
- Change to `bool selectionModeEnabled = false;`.

---

## Notes

- **Continuous Play Mode** (long-press a letter) is unaffected by selection mode and works as before.
- The **light show is non-blocking** — playback timing and other critical functions continue normally during the 7-second display.
- **Selection mode resets** automatically when the device resets or reboots; queued songs are saved to EEPROM as before.
