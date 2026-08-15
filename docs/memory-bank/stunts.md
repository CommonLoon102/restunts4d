# What is Stunts?

Stunts (AKA 4D Sports Driving) is a video game
created in 1990-1991 for MS-DOS. The player is racing through a track which
contains many stunt elements (corkscrew, chicanes, slalom, jump ramp, etc.).

The game features:
- 6 AI opponents
- A track editor
- Saving highscores for the tracks
- Race against time only (when no AI operated opponent selected)
- 11 cars what you can select to race with (you can choose same or different car for your opponent)
- Can add more cars to the game by copying new files to the games's folder
- Replay saving/loading/playback

There can be only one opponent selected to race against - or no opponent at all.
Each opponent have different caracteristics, some of them are more skilled then
others. They have strenghts and weaknesses (what kind of stunts they are good at).

## Replay recording

The player (user) can decide to save the replay of his run on the track.
The replay stores the track and all the inputs what the player done.
In each second 20 times the inputs are collected and stored.

### Replay handling

During the race, the player can go to the game menu by pressing esape and
watch the replay, seek in the replay to anywhere. It is possible to rewind
the replay a little and continue driving from there. The game warns you that
doing this won't allow you to get onto the highscores, because this is considered
cheating. But there is a bug in the game to avoid the detection of replay handling:
just save the replay, go back to main menu, load the replay and finish the track,
and you can get onto the highscores.

## Bugs in the game

The physics in the game are very basic and not realistic at all. The game is only
using integer calculations due to slow hardware in 1990, no floating-point
co-processors at that time. Due to this and many programming mistakes there are many
bugs in the game, but this was accepted and exploited in the community, so now the bugs
are features of the game. The collision detection is very buggy in the game, players
are exploiting it to just go through obstacles intead of avoiding them.

### The slalom bug

Hitting a slalom block head-on may result in the car tunelling directly through it and emerging safely on the other side. To succeed in deploying the slalom bug, the car must:

 - Have sufficient speed (around 145 mph at a minimum, getting progressively easier at higher speeds);
 - Be moving at a direction very nearly parallel to the yellow line;
 - Hit the front face of the block with its full width.

### Fast grass

Fast grass is the grass around a chicane. This grass is called fast because, due to a game bug, it functions as road although it looks like grass. You can thus drive as fast on the grass around a chicane as you can on the chicane road or any other road. Due to this property, chicanes are useful resources to track designers for creating alternative racing lines.
In Stunts, the term chicane refers to the fast chicanes available as a 2x2 stunt element for track building. Although they were intended as a tricky and potentially exhilarating test of high-speed handling, it is not possible to appreciate these traits at their fullest extent due to a rather glaring oversight which makes the "grass" surface around a chicane behave as regular tarmac. For that reason, chicane grass is commonly referred to as fast grass. Fast grass not only makes it much easier to take a chicane, but also allow for some interesting track building tricks. Chicanes are very often employed as wide landing strips for long jumps and as bypasses around slow track combos. Not only grass, but water is also affected by this trait, making the fast grass bug even more relevant.

### Up/Down Corkscrew (Loop) bug

Also possible to just pass through this object by hitting it in the correct angle
and speed, therefore the obstacle is skippable.
To do a bug loopcut, one has to shoot directly against the back of the descending leg of the loop and hope the car will go through it. High speeds (above ~150mph) are required for pulling it off; however, the exact speed to make it work is completely dependant on how you are hitting the loop in a particular lap (often getting a few mph faster or slower will do the trick). The trick is also quite sensitive to car positioning - better results are usually obtained with the car pointing nearly straight and only slightly to the right of the midpoint of the loop.

There are two track elements called "corkscrew" in Stunts:
 - The corkscrew left/right, a stunt element which spirals around the track direction axis;
 - The corkscrew up/down, a ramp which spirals along the vertical axis.

### Power Gear bug
Some cars in stunts have the ability to reach a so-called power gear or 6th gear, or simply PG. This usually happens while driving in the last official gear with a high RPM, and doing a jump. Some cars can reach it from any speed, when driving on a loop, and shifting to the last gear.

Power gear allows cars to go a lot faster than the given maximum speed and, crucially, not even slow down on grass when driving in power gear. These are: Porsche March INDY, Ferrari GTO, Corvette ZR1 and Acura NSX. There are some important differences between Power Gear behaviour of those cars:

 - The INDY, Ferrari and Corvette go 245 mph in 6th gear, while the Acura only goes 203 mph.
 - The INDY and the Acura can reach power gear from any speed using a loop, shifting up to 5th gear before reaching halfway through it.
 - Also, INDY and Acura can reach Power Gear on a jump before getting airborne, or while driving around a left/right corkscrew.
 - Unlike the other mentioned cars, the Acura is unable to do slide turns on grass or road while in Power Gear - attempting to do so will get the car out of Power Gear. (see, however, Helicopter trick)

In general, power gear cars can be grouped in two categories: rigid power gear cars, which attain power gear exclusively by reaching speeds higher than ~225mph, and flexible power gear cars, which can reach power gear as they are driven through surface changes (such as jumps and loops), the minimum speed for that to happen being dependant on both car and surface characteristics (and generally much lower than 225mph). Ferrari and Corvette have rigid power gear, while Indy and Acura have flexible ones.

For all four cars, power gear also allows the Fence Sliding trick on water.

As power gear allows for sustaining very high speeds for extended periods of time as well as much more radical grass shortcuts, finding the power gear spots on a competition track is usually the key to victory with PG cars.

### Fence Sliding

An advanced trick, which allows you to navigate better in power gear. You use the edge-of-map fence to change direction by sliding along it. You can drive into the fence, keeping power gear, and then leave it when you've reached the right place. It is possible to fence-slide across water, which makes for surprising shortcuts on some tracks.

### Helicopter Trick

Advanced trick of Acura NSX. When Acura is in powergear, a certain kind of hard steering forces the car to make a huge bug jump with rotation and after landing it keeps quite high speed. It can be very useful for cutting sharp corners. Angle of rotation can be various.

### Magic Carpet

This is the expression used for when your car makes a spontaneous jump from flat ground. Magic Carpets are an unpredictable wildcard in competition races, as they usually allow the racer to save time by keeping a higher speed, sometimes even reaching or keeping power gear where it would not otherwise be possible. The random nature of magic carpets makes them hard to use and an unpredictability factor, sometimes allowing weaker racers to beat stronger racers.

### Dual-way switching

Dual-way switching is a kind of shortcut that exploits a programming oversight in Stunts' penalty time system. On tracks where the road splits, it is possible to leave one of the paths and re-enter the track through any point of the other one without penalty time, provided at least one track element is crossed before the paths rejoin. Tracks with multiple paths usually either are designed in a way to prevent dual-way switching or explicitly exploit them to make alternative racing lines available.

## Penalty time

Penalty time is the built-in mechanism of Stunts to curb shortcut usage. It ensures that, in most circumstances, skipping more than two track elements consecutively will lead to several seconds being added to the final lap time. The penalty is calculated in real time, and displayed to the racer mid-lap as soon as it is calculated if the in-car (F1) view is selected.

### Penalty time detection

As penalty time is calculated on the fly as a lap is driven or a replay is ran, there is currently no way to detect penalty time just by checking a replay file. Such an inconvenient affects both replay checking software like RPLINFO and automated competition management systems such as the one used by ZakStunts - in the latter case, managers have to rely on the racers reporting correct penalty times if the scoreboards are to be consistent throughout the month. Porting the game engine is the only realistic hope of improvements to this situation.

### Penalty rules

Below follows a reasonably complete explanation of how to find the penalty time for an arbitrary cut. If all you want to know is how to avoid penalty, things are much simpler: do not skip more than two track elements consecutively and do not miss the last track element before the start/finish tile.

Consider a racer which left the track and then rejoined it at some further point. Let us call the beginning and end points of the shortcut exit element and re-entry element, and everything in between skipped elements. Finally, by penalty chain we will refer to the elements counted in the calculation of penalty time. Under normal circumstances, the penalty chain includes:

 - the skipped elements,
 - the re-entry element, and
 - the element coming immediately after it.

Therefore, if, e.g, five elements are skipped the penalty chain will usually have seven elements. One easy way to identify the end of the penalty chain is that the penalty time message will be shown to the racer when the chain is closed, i.e., upon leaving its last element and entering the following track element.

Still assuming normal circumstances, penalty time will be given whenever there are more than two skipped elements in the penalty chain. The value of penalty attributed will be three seconds for each element in the chain if that results in more than 18s, and 18s otherwise. That explains why 18s is the most common penalty value: to get more, one typically has to skip at least 21/3 - 2 = 5 elements.

Abnormal circumstances, as opposed to the "normal circumstances" referred above, include:

 - If the start/finish tile would be met anywhere in a penalty chain, the chain ends just before the finish tile. Furthermore, any accumulated penalty will be added, even if less than three elements were skipped. For that reason, skipping the last element(s) before the finish line will always lead to penalty, in the amount of three seconds per skipped element.

 - If, after performing a penalty-inducing shortcut, one returns to the track and then leaves it immediately, without closing the penalty chain, penalty will be calculated as if all elements between the first leaving of the track and the final re-entry had been skipped, even those crossed upon the temporary rejoining of the track.

- There are many subtle exceptions to these rules when crossroads, dual-way splits and rejoins are involved; such edge cases demand further investigation. Just to mention one of the simpler examples, skipped dual-way split tiles do not count towards penalty computation if one rejoins the track through the non-straight path.

## Map editing

With the in-game track editor you can place track elemens and modify the terrain.

### Flooded track

Thanks to external track editors such as Track Blaster or Bliss, it is possible to place any track element on water terrain. That opens up the possibility for building the so-called flooded tracks, in which grass is partially or wholly replaced with water. Flooded tracks and sections are commonly used in competitions as a way to restrict shortcut possibilities.

#### Limitations

Multi-tile elements such as large corners and loops will, depending on their orientation, look like they are on grass if placed on water. The effect is of the same nature than those achieved by terrain manipulation in illusion tracks. In most cases, however, the effect is merely visual, and the area surrounding the track will behave as water. The sole exception are chicanes on water; the flooded parts of the chicane behave as asphalt due to the fast grass bug. That actually makes chicanes useful as a way to make flooded track sections easier.

