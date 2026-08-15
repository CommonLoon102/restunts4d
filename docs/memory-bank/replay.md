*Replay File Format* refers to the format of binary '.rpl' files used by Stunts

## Overview
Replays are saved by Stunts as binary files of extension *.rpl*. They contain the necessary information to recreate the actions by the player on one track, with one car and to identify the opponent. Replays are always saved from the start of the race. It's not possible to have a replay starting any later. All moves made by the player are saved up to a maximum of ten minutes at maximum quality (20 frames per second).

Actions by the opponent (if any) are not saved. Stunts will reproduce the opponent's AI during the execution of the replay instead. This means that, if a custom AI were created, the AI file should be passed along with the replay if there are opponent moves expected to be shown with accuracy.

Because the whole track and terrain are saved within a replay file, it is possible to reproduce it without having the [track file]. It's also straightforward to extract the track from the replay file.

Two replay formats exists. The "old format", used by the 1990 and Mindscape 4D Sports Driving 1990-12-13), and the "new format", used by all others. They only differ in two additional bytes added to the latter, but because the physics engine does change from a version to another, the same moves will not yield the same results. Therefore, replay formats across Stunts versions are considered *incompatible*.

## Header
* (4 bytes) Car ID of player's car
* (1 byte) Car colour code of player's car
* (1 byte) Transmission or player's car (0 = manual, 1 = automatic)
* (1 byte) Opponent ID
* (4 byte) Car ID of opponent's car
* (1 byte) Car colour code of opponent's car
* (1 byte) Tranmission of opponent's car (0 = manual, 1 = automatic)
* (9 bytes) Name of the track file used to record, right padded with null bytes
* (1 word) Play frequency (tics per second)  <<-- Not present in v1.0
* (1 word) Number of ticks in replay

**Car IDs** are the four characters (usually alphanumeric) that identify a car's files. For the game's original cars, these are as follows:
* COUN : 25th Aniversary Lamborghini Countach
* FGTO : Ferrari GTO
* JAGU : Jaguar XJR9 IMSA
* LANC : Lancia Delta HF Integrale 16V
* LM02 : Lamborghini LM-002
* P962 : Porsche 962
* PC04 : Porsche Carrera 4
* PMIN : Porsche March Indy
* VETT : Corvette ZR1
* ANSX : Acura NSX
* AUDI : Audi Quattro Sport
When playing solo, the opponent's car ID is set to a character 255 followed by three null bytes.

**Colour codes** depend on the car. Each car has a number of paintjobs, which is always 7 for the original game cars. The paintjob to be used is identified with a number from zero to one less than the number of paintjobs. For any of the original cars, a number greater than 6 here will be assumed 0. Custom cars may have more than 7 paintjobs. What happens with higher numbers in that case is untested. A curiousity is that the Porsche 962 and the 25th Aniversary Lamborghini Countach are the only two original cars with the exact same set of paintjobs. When playing solo, the opponent's car colour is set to zero.

**Opponent ID** goes from 0 to 6. A null here means no opponent. For values 1 to 6, the default opponents are the following:
* 1 : Squealin' Bernie Rubber
* 2 : Herr Otto Partz
* 3 : Smokin' Joe Stallin
* 4 : Cherry Chassis
* 5 : Helen Wheels
* 6 : Skid Vicious
Using any larger number causes Stunts to try to locate a file that's not found, suggesting it is possible to create new opponents.

**Play frequency** is normally 20 tics per second, but Stunts will drop this to 10 if the CPU is to slow and can't keep up. Although Stunts doesn't seem to use any other frequency, modifying this value to anything else does work. A normal replay at 20 tics per second that gets this byte modified will appear to run at a different pace. Crazy speeds are possible this way. For v1.0, only 20 tics per second can be handled. When recording a replay at 10 frames per second, it should theoretically be possible to exceed the 10 minutes maximum length, given that it takes up half as much space. In practice, what happens is that Stunts will not warn past this threshold and it will still be possible to return to the replay menu, save, load, etc. Yet, it appears that some memory overrun *does* occur so the game becomes unstable. In a test, Stunts ended up abnormally after a replay somewhat longer than 10 minutes with an error: *locateshape - car0 SHAPE NOT FOUND // locatesound - n-Ti SOUND NOT FOUND // reservememory - OUT OF MEMORY RESERVING t P=2e75 HW=cbe0*.

## Body
Following the header, is an exact copy of the raw original [track file], which takes up 1802 bytes. Right after that, at offset 0x724 in the new format, the replay proper begins. One byte is dedicated to each tic. On each tic, the relevant actions (typically, key presses) are recorded:

* bit 0 : Accelerate
* bit 1 : Brake
* bit 2 : Turn right
* bit 3 : Turn left
* bit 4 : Gear up
* bit 5 : Gear down
* bit 6 : Ignored
* bit 7 : Ignored

Modifying bits 6 and 7 has no effect. There's also no effect in changing bits 4 and 5 if the player is using automatic transmission.

## Recognising the format
One could use a simple method to very accurately assert on which format the replay is:
* Assume new format and read the word that should contain the number of ticks
* Add to this number 26, for the header length and 1802, for the track length
* Compare with the replay file length. If equal, it's a new format replay file
* Otherwise, now assume old format and read the word again at position two bytes before
* Add 24, for the header length and 1802, for the track length
* Again, compare with the replay file lenght. If equal, it's an old format replay file
* Otherwise, there's something wrong with this file

## Calculating the replay length in time
Dividing the number of ticks in the replay by the frequency (which is always 20 for the old format and is specified in the new format), we get the number of seconds (with decimals) that correspond to the total length of the replay. This is not necessarily the same as the racing time. Normally, if the race is completed and the game is allowed to record till the end, the replay will be exactly one second more than the racing time. However, if the player crashed or interrupted the replay, this difference will not hold.

A hint to whether the replay ended with the player completing the race can be gathered by examining the bytes that compose the last second (last 20 bytes for a frequency of 20). If the replay ended normally (getting to the start/finish line and allowing the recording to complete), then all these bytes should be zero, as Stunts ignores and does not record any key presses after having crossed the finish line. Yet, there is a chance that the player simply didn't press any key in the last second of his recording. For this reason, the only way to be certain is to actually have a human watch the replay.

