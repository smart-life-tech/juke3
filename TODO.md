- [ ] Modify playSong() function to always turn off all LEDs except the current pair when a song starts playing
- [ ] Remove the conditional check for the last song in the queue for LED control
- [ ] Test the changes by uploading to Arduino and verifying LED behavior
Code 222:
Utilize other functions in previous codes (3-song limit, long-press-activated continuous play,
etc.)
Key differences:
1) It has 10 letters (ABCDEFGHK) and 10 numbers (1234567890) respectively.
2) Tracks are limited to 100 songs
3) Each song is played by selecting one letter and one number.
4) Letter/Number buttons remain lit until the 3rd selection is made
5) After the 3rd selection is entered, all button LEDs go off except for the letter/ number of
the current track playing.
6) After the 3rd song is completed, all LEDs light back up.
7) For testing purposes, leave off the Nayax function.
8) Add a failsafe of only "Letter-Number" order (if a number is pressed and no letter is
detected prior, Arduino ignores the number)