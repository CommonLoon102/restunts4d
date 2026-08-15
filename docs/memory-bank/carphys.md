# Car model physics

This file intends to discuss in depth the parameters affecting how a car behaves in Stunts and, in particular, expand on the descriptions done at Car parameters. The first section consists in an analysis of how performance-related CAR*.RES parameters quantitatively affect in-game behaviour, done through the definition of a consistent measurement unit system. Such an unit system proves to be an useful framework for accurate, realistic tuning, and allows to assess how accurate the original Stunts cars were modelled by the developers. Further sections of the article deal with other, less directly assessable parameters such as grip as well as special effects. As a final observation, the largely experiment-based nature of the topic means this article will be a work-in-progress for quite a while.

## Model fundamentals and internal units

This section is organized by the main physical quantities relevant to the behaviour of a Stunts car. For each of them, the essential physical equations tuners need to be aware of will be discussed, alongside with the unit correlations which allow translating real-world data to CAR*.RES byte values. The general treatment will be attributing to each quantity an internal unit which can be read directly from the game or CAR*.RES and then work out the corresponding real-world units.

## Time

As it appears to the drivers, time in Stunts is divided in steps of 0.05s, corresponding to the interval between successive updates of the car position. For convenience, a TS (for time step) unit will be defined, so that 1TS = 0.05s . Also, it's worth to remind that 1min = 60s = 1200TS, and 1h = 3600s = 72000TS.

## Speed

Speed values in miles per hour are readily available, either through digital speedometer readings or evaluation screen top speed/impact speed data. Note that the reported values are rounded down to the nearest integer, which puts a limit to the accuracy of various kinds of in-game experiments. Speed plays a crucial role in applying real units to Stunts physics, as it links time, which we know exactly in-game, and length, which is much harder to correlate with reality.

## Length

There are two main reference systems for defining length units in Stunts:

 - Track tiles, which provide are a natural unit for a driver. We will use tl as the symbol for the length of a tile, with the track area, for instance, being a 30tl x 30tl grid.

 - 3D shape sizes, which give us relative sizes for the objects in the game. The graphic size unit in resources such as car1 and all objects at GAME*.3SH will be referred to as point, or pt. Inspection with stressed readily shows that a 1tl = 1024pt. The correlation between speed and length revealed by knowledge of the game internals implies that 1pt = 0.2ft, exactly. Furthermore, speed can be recast in internal game units using 1mph = (11/30)(pt/TS). These observations agree with the results of older, in-game experiments to find the conversion factor.
 Those experiments consist in racing a car down a long straight at a constant and known speed and infer the travelled distance from the speed and the time elapsed. Performing these tests with high accuracy is not trivial, however, and usually requires CarBlaster trickery to lock the car into the desired speed. Repeating such runs under different conditions gave a result of 1tl = 205ft, only marginally different from the real value of 1tl = (1024/5)ft.

As a final note, it is useful to have in mind that 1mi = 5280ft = 2^5*5*3*11ft.

## Engine revolutions

It is quite evident from mere inspection that the engine speed parameters in CAR*.RES are given directly in rpm. Toying with maximum rpm parameters and the torque curve allows to prove beyond doubt that each byte in the torque curve corresponds to a 128rpm range. There is not much to add in this topic, except for mentioning that the conversion to SI units, 1rpm = (2*pi/60)(rad/s), may occasionally be of some use.

## Gear ratios

Gears couple the engine speed and delivered torque to the wheels and, therefore, to actual car movement. The key formulas to be aware of are:

```
car speed = engine speed / overall gear ratio
car acceleration = engine output torque * overall gear ratio / car mass
```

The "overall gear ratio" (that is, the gear ratio parameter we're used to) incorporate the effects of all transmission components that would be in a real car (gearbox, final drive, wheel radius...) into a single value.

As the gear ratio couples engine speed to car speed, the internal unit of the gear ratio parameters (from now on the gear Stunts unit, or gsu) must have dimensions of angular (engine) speed per linear (car) speed. Finding the conversion factor requires just finding out what the car speed is at a certain rpm and ratio, something easily achieved by letting the car reach maximum rpm or, for better accuracy, manipulating the torque curve to lock the engine speed at a fixed value. The final result is incredibly simple: 1gsu = (1/256)(rpm/mph). Finding the car speed at some rpm, therefore, just requires dividing the rpm by the gear ratio as read from CAR*.RES and multiplying by 256. Using the relation to figure out the correct gear ratios from the desired maximum speed at a certain gear is just as straightforward. The simplicity of the conversion suggests the game actually uses it at some point of the algorithms.

## Torque and mass

Finding out how to convert real torque curve shapes into a Stunts torque curve is probably the most interesting use for the knowledge of Stunts internal units. It is also a more tricky task than the ones presented before, since not only accurate tests for measuring acceleration require careful design but also there's the added complication of having the mass parameter involved as well. To see why, consider the equation:

```
car acceleration = engine output torque * overall gear ratio / car mass
```

This is valid as long as there is no aerodynamic resistance (just a question of setting the drag coefficient to zero). Assuming the engine delivers constant torque (and thus acceleration is constant), and using the fact that acceleration = speed change / time elapsed , we can rearrange it into:

```
engine output torque / car mass = speed change / (time elapsed * overall gear ratio)
```

Both speed change and time elapsed are known in real-world units (mph and s), and the internal unit for the gear ratio (gsu) is known too. If the internal mass unit (from now on msu) was known as well it would be trivial to substitute everything and find what the internal torque unit (tsu) is. But since both units are not known and it is impossible to find an equation in which they appear uncoupled it becomes impossible to find them both from in-game data only. The best that can be done is to find the ratio tsu/msu. A value of 1(tsu/msu) = (1/330)mph^2/(rpm * TS) was obtained after several carefully-done runs which gave values very close to this simple fraction. The presence of a multiple of 11 in the result is very suggestive, since the conversion from miles to feet (and therefore to the internal length units) also involve a factor of 11.

## Aerodynamic drag

Air resistance is the main opposing force acting on an accelerating car at relatively high speeds, and thus is an effect of critical importance. Indeed, for a Stunts car speeding through the track, it is reasonable to consider drag as the only significant resistance acting on the car. How the 5Eh/5Fh drag parameter correlates with car behaviour is the last missing piece of information needed to predict from parameters and model from external data straight line car performance with reasonable accuracy.

Aerodynamic drag is most often modelled as a force proportional to the square of car velocity. The simplest way of stating that would be:

```
drag force = drag constant * (car speed)^2
```

The "drag constant" referred here actually includes a number of terms mostly related to the car shape (check Physics of Racing for details). While analysing how this speed-dependent force would affect car acceleration in order to estimate the constant value would be very troublesome, an easier approach makes use of a very important effect of drag: it plays a major role in defining the straight line top speed. Admitting the car is only affected by engine propulsion and drag, top speed will be reached when those forces cancel each other, that is:

```
engine force = drag constant * (car speed)^2 			=>
torque * overall gear ratio = drag constant * (car speed)^2 	=>
(car speed)^2 = torque * overall gear ratio / drag constant 	=>
car speed = square root{torque * overall gear ratio / drag constant}
```

If our considerations are correct, top speed at a flat straightline should increase proportionally to the square root of engine torque, and decrease in inverse proportion to the drag constant, which in turn can hopefully be identified with the 5Eh/5Fh parameter. Thankfully, that is exactly what happens.

As for finding the internal unit for the constant (dsu), the process is made much simpler by noticing that, at the straightline top speed, the product torque * overall gear ratio is exactly half of drag constant * (car speed)^2. That leads to the unit equation 1dsu = (1/2) * tsu * gsu/(mph^2). This simple relation in internal units, useful enough for most tuning purposes, can be refined further to 1dsu = (1/64)(lbf * ft * rpm/mph^3) ~= 0.00834lb/ft ~= 0.0124kg/m.

# Additional parameters and effects

## Braking

Brakes in Stunts are a rather straightforward matter. Bytes 2Ah/2Bh define a linear scale of acceleration opposing to the car's movement that determines effectiveness of brakes. With 2Ah = 0 and 2Bh = 1 a car without aero drag will go from 169mph to a full stop in 8.45s. That corresponds to a deceleration of 8.94m/(s^2), or 0.911g (1g = gravity acceleration at sea level); naturally, increments to 2Bh correspond to 1/256 of that amount. The use of acceleration instead of force here is not an oversight: Stunts does not account for mass for braking! Aerodynamic drag will also slow down a car whenever the accelerator is not pushed (at least while the car is planted to the ground), that's why one gets the impression that brakes are more effective at higher speeds. Finally, a curious point is that if both brakes and aero drag are set to zero a car running on straight road will not slow down no matter what you do, due to the complete lack of opposing forces...

## Cornering grip

Doing accurate estimates for cornering grip is rather more difficult than for other parameters, since there are variables that are a bit hard to control, the most important of them being test driver precision... The simplest method of measurement is to pick a flat, tarmac large corner and try to find the highest constant speed at which it is possible to complete the corner following the centerline (approximate radius of 93.1m) without sliding/skidding. For CAh = 0 and CBh = 1, that speed is approximately 74mph. That information allows one to find maximum centripetal acceleration available, which is a good enough reflection of maximum grip for our purposes. The equation for centripetal acceleration is:

```
centripetal acceleration = (cornering speed)^2 / corner radius
```

Substituting our data gives a centripetal acceleration of 11.758m/(s^2), or 1.198g, for CAh = 0 and CBh = 1. The grip scale is nearly linear, but not exactly, as can be proven by setting zero grip and repeating the test. Cornering without skidding is still possible, even if at 7mph... that may hint on the existence of a very minor secondary factor affecting cornering grip. Notice that again there was no mention of mass - the cornering model does not take it into account to as it was supposed, which is somewhat disappointing... Banked corners of course increase centripetal acceleration, thus allowing higher corner speeds. For the same grip discussed just above, a banked turn can be taken on its centerline at 104mph. That is a 40.5% increase on cornering speed, corresponding to a massive 97.5% increase in centripetal acceleration. Such a large growth in relation to the ~1,2g flat corner grip would require, in reality, a banking of about 50 (!) degrees, obviously much more than in-game graphics display. Of course, this discussion is immaterial, since mass does not affect grip in bankings either, while mechanics tell us the very fact cornering is easier on positive-banked turns is due to a reaction force to the car's weight...

## Grass slowdown

The slowdown cars suffer when running on grass resembles aero drag, in that both define limit speeds and, therefore, scale up with car speed. A closer inspection on grass slowdown reveals it differs from drag in that the force is proportional to speed, instead of to the square of the speed (proof is given by verifying limit speeds on grass grow linearly with gear ratio). Moreover, the limit speed is proportional to car mass, indicating yet another effect described in terms of accelerations and not forces. A test car set with 40tsu flat torque curve, 100gsu gear ratio and 12msu mass reaches 63.5mph (that is, a digital speedometer oscillates constantly between 63mph and 64mph). The relevant equation would be:

```
engine force = grass slowdown force 								=>
engine torque * overalll gear ratio = grass deceleration * car mass 				=>
engine torque * overalll gear ratio = grass deceleration constant * car speed * car mass
```

Substituting the values gives 0.1444(1/s) value for the deceleration constant. A word of caution, though, that this result is not fully proved to be universal, not at least until someone fully dissects CAR*.RES and proves grass slowdown is not regulated by some odd unknown parameter.