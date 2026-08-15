# Car parameters

This article discuss how the car*.res parameters control the performance, handling, aesthetics and other aspects of a Stunts car. The parameters are grouped by function, in order to allow more fluent reading and discussion. For a deeper discussion on the inner workings of physical parameters, refer to Car model physics (carphys.md).

## Preliminary notes

### Hexadecimal numbers

In order to read and edit the parameter values, you will need to make decimal/hex conversions. Most familiar software calculators are able to perform the conversion as well as the basic operations on hex numbers, so that should not be a problem. If you wish to do the conversions by hand or mentally, remember that hex digits go from 0 to 15 (with letters A...F standing for 10...15), and that each digit to the left is one power of 16 higher (instead of a power of 10 as with decimal numbers). For instance, 17C9h = 1*16^3 + 7*16^2 + 12*16^1 + 9 = 6089 .

### Car Blaster conventions

Unlike regular hex editors, Car Blaster defaults to displaying both byte addresses and byte values in decimal values. That can be an inconvenience, so we will not stick to that convention. Within this article, byte addresses will always be quoted as hexadecimal numbers. Byte and parameter values can be quoted both ways depending on the situation, so for clarity an "h" will be appended to all hexadecimal numbers. It is simple to switch between hex and decimal display in Car Blaster - just press 'h' for switching the addresses or 'Shift+h' for switching the values.

### Parameter types

Unlike Car Blaster interface suggets, in most cases the values of the parameters do not correspond to a single byte in the CAR*.RES. They may be stored in a few different ways, which we'll refer to as data types. Understanding them is key to reading the values correctly. A description of them is provided below:

#### Integers

The most common and important data type. Integer parameters occupy *two* bytes in the file, and they should be read together to form the value in the following way:

* Pick the byte values *in hexadecimal* and append one to the other *in inverted order* (second byte comes first). Doing so will give you a four-digit hex number (the technical term for this representation is [little endian](http://en.wikipedia.org/wiki/Endianness)).
** If the number is 7FFFh (=32767) or smaller, it is the parameter value.
** If it is larger than 7FFFh, however, the parameter is not a very large integer number, but rather a *negative* integer stored in a particular way (called the [two's complement](http://en.wikipedia.org/wiki/Two's_complement)). To recover the real value, subtract the value you read from the file from 10000h (=65536) and invert the sign of the result.
* The value thus obtained is the actual parameter value, and can be used as you wish, either in hex or converted to decimal.
* Two examples: Let's say an integer parameter has byte values of 18 and 2C. Grouping the values in inverted order gives 2C18h, which is smaller than 7FFFh. Therefore, the parameter value is 2C18h = 11288 . Now, another integer parameter might have values of 40 and FC. Grouping gives FC40h, which is larger than 7FFFh. Therefore, the real value is the two's complement, -(10000h - FC40h) = -03C0h = -960 .

From the above description, it can be inferred that an alternate way of reading the values is to sum, in decimal, the value of the first byte to the second one multiplied by 256 (and then taking the two's complement if the value is greater then 32767). Doing so avoids dealing with hexadecimals, but is far less convenient if you have an hex calculator available. A practical remark is that the second byte changes the parameter value at a rate 256 times faster than the first one. For some parameters, the second byte may thus be though of as the "main" one, and the first one as a "fine adjustment". For other parameters, though, the second parameter defaults to zero, and a change of 256 units may be exaggerate for most useful purposes, and therefore we will be most of the time interested on the first byte. A final remark is that negative integer parameters are usually easy to recognize in Car Blaster through the very large second byte values.

#### Unsigned integers

These are the same as integers, except they are always positive, and so you don't need to worry about taking the two's complement for large values. Else, all remarks made above still hold.

#### Bytes

While most parameters are 2-byte integers, some are nevertheless stored as single bytes. These are naturally a lot simpler to handle - just read the value directly, either in hex or in decimal. Single byte parameters are always unsigned.

#### Pairs and triplets

When there is need to represent the position of a point, pairs and triplets of values are employed. For 2D coordinates, defined over a bitmap, a pair of consecutive values is employed, standing for horizontal (x) and vertical (y) coordinates. For 3D coordinates on the "physical world", triplets are used instead, with values standing for x-y-z (width, height and length) coordinates in that order. The values themselves may be integers or single bytes, depending on each case. Pairs and triplets will be represented on this article by enclosing the values in brackets.

## Engine and Transmission

### Torque curve

*Data type: **Byte** (x103)*

*Address: **3Bh...A1h** (simd); **61h...C7h** (absolute)*

Defining how much acceleration the engine can impart on a car at every given engine speed (rpm), the torque curve is a very important factor in determining performance. Every byte navigated forward corresponds to increments of 128rpm, so that the first byte covers 0...127rpm; the next one, 128...255rpm and so on. There are 103 bytes in total, and so the engine can deliver power over a range of 13184rpm. The curve works as expected: raising a byte increases linearly acceleration given by the engine at a given rpm. A more peaky torque curve means a narrower range of useful engine speeds - and thus, the driver will have to shift more often.

### Idle rpm torque

*Data type: **Byte***

*Address: **3Ah** (simd); **60h** (absolute)*

This may be thought as a special point in the torque curve. It overrides a section of the curve at low rpms, in order to better represent the car launch from a standstill. The affected range is 0...2559rpm (the first 20 bytes of the curve) for the 1st gear, and 0...''value of idle rpm'' (described further down the article) for 2nd gear and above.

### Number of gears

*Data type: **Byte***

*Address: **00h** (simd); **26h** (absolute)*

Self-explanatory. Acceptable values range from 1 to 6. If you plan to add a 6th gear to a car, remember to set all other gearing parameters (which we will describe shortly) for the sixth gear as well.

### Downshift rpm

*Data type: **Integer***

*Address: **08h/09h** (simd); **2Eh/2Fh** (absolute)*

This is the downshift rpm point used by the automatic transmission. For optimal performance, it should match the rpm position of the torque curve's peak. *Observation for all rpm parameters:* the parameter values gives the actual number of rpm directly.

### Upshift rpm

*Data type: **Integer***

*Address: **0Ah/0Bh** (simd); **30h/31h** (absolute)*

This is the upshift rpm point used by the automatic transmission. For optimal performance, it should match the rpm position of the power curve's peak - remember that, at any given rpm, power = torque * rpm; you can use that relation to estimate a power curve from the torque curve.

### Maximum rpm

*Data type: **Integer***

*Address: **0Ch/0Dh** (simd); **32h/33h** (absolute)*

This parameter is the maximum rpm (the "redline") of the engine. Whenever the engine goes beyond this value, the rpm value will be reduced at the next time step (1/20th of a second) so that it fits again within the limit (such behaviour explains why the rev meter oscillates when the maximum rpm is reached). Naturally, this value should be adjusted whenever the torque curve is extended to higher rpms. A less obvious fact is that, as there are no other limits to the maximum rpm, this parameter can be set to an arbitrary high value (well beyond the end of the torque curve or even the maximum size of the torque curve if you wish); and, in that case, all engine speeds (and thus car speeds) made available can be reached by jumping even if there is no torque available on the rpm range. As a very important consequence, the maximum rpm defines, together with the gear ratios, the overall top speed of the car.

### Idle rpm

*Data type: **Integer***

*Address: **06h/07h** (simd); **2Ch/2Dh** (absolute)*

The so-called "idle rpm" modulates the engine behaviour at low rpms. The main function of the parameter is to define up to which rpm value the "idle rpm torque" will be used instead of the regular torque curve for the second gear and above. In addition, the idle rpm sets the "sensory" indicators of the engine speed (rev. meter position and engine sound pitch) to a fixed value from zero rpm to its value (since car speed and engine speed are always coupled, the ''actual'' rpm value is not fixed, however). If this value is set higher than the maximum rpm, the car will be unable to move. The values can be made equal, though - that is an useful trick to make a constant-torque engine over an arbitrary range of engine speeds if you don't mind having an useless rev meter and an annoying high-pitched engine buzzing endlessly.

### Gear ratios

*Data type: **Unsigned integer** (x6)*

*Address: **10h/11h...1Ah/1Bh** (simd); **36h/37h...40h/41h** (absolute)*

These parameters set the gear ratios of the car, and are therefore very important ones. The first word of two bytes sets 1st gear, the following word, the second one, and so on. The gear ratios are overall values, representing the effects both the gearbox and the final drive gears as well as those of the wheel radius. As it could be expected, a higher value means a shorter ratio, and thus higher acceleration but less final car speed in a certain gear (at any rpm the torque delivered to the wheels is proportional to gear ratio, but wheel speed is inversely proportional). The exact interpretation of the values is straightforward:
 car_speed_mph = 256*engine_speed_rpm/gear_ratio
This is by far the most useful equation to know when setting the parameters of a car, as it allows to predict car speeds in downshift/upshift points and real top speeds for each gear as well as finding the torque on each. A realistic car needs of course to have decreasing gear ratios, and for smoother engine operation a near-exponential decrease would be appropriate.

## Physics parameters

### Car mass

*Data type: **Integer***

*Address: **02h/03h** (simd); **28h/29h** (absolute)*

This parameter had been variously described over the years as "inverse power amplification" (Mark Nailwood in Car Blaster) or "aerodynamic resistance" (Juha Liukkonen / Lukas Loehrer). Were it an aerodynamic coefficient effect, however, it would affect car more strongly at high speeds. Raising the parameter causes a decrease in acceleration directly proportional to the increase, all gears being equally affected. Therefore, it is best understood as being the car mass. Clearly, it has crucial importance to the car performance. The original Stunts cars have masses ranging from 15 to 55 (in decimal); the second byte defaults to zero and increasing it raises the mass way too fast, so it is usually disregarded. Another very important fact about mass - almost as important as the regular physical effect - is that its value sets the kind of Power Gear or Anti-Power Gear, if any, that the car will have (see Power gear bug).

### Braking effectiveness

*Data type: **Integer***

*Address: **04h/05h** (simd); **2Ah/2Bh** (absolute)*

As the name says, those addresses tell how powerful the car brakes will be. The effect of the brakes is proportional to the value - any apparent deviations from the prediction are in fact effects of aerodynamic resistance. Typical values are close to 0100h (=256), except for racing cars, which may have significantly better brakes. Setting the parameter to *0000*h will turn them off completely, and thus the car will only be stopped by aero drag and, possibly, grass slowdown. Other parameters, such as mass or grip, have no effect on braking.

### Aero Drag

*Data type: **Integer***

*Address: **38h/39h** (simd); **5Eh/5Fh** (absolute)*

This elusive parameter controls the aerodynamic resistance (drag force) encountered by the car when accelerating down a straight. Drag force increases quadratically (that is, proportional to the square of) with speed, up to the point the engine can't produce enough torque to overcome it and the car reaches its top speed. Therefore, increasing the drag parameter will lower the car top speed in a flat straight line (that is, no jumps or [[Power Gear]]), as well as significantly reduce acceleration at high speeds. The parameter value (Drag) is directly proportional to the drag force. It can be verified that for a given set of values for gear ratio, torque and Drag parameter, the maximum possible speed value will be given by:
 max_speed_mph = sqrt(2*torque*gear_ratio/drag)
This relation allows to predict the flat track top speed of a car.

The Drag parameters also determines how fast the car will lose speed when the accelerator is not being held. It should, additionally, make the car slow down whenever it reached speeds higher than the flat-track top speed (when landing from a jump, for instance). Due to the same bug which originates powergear, however, this effect is observed, among the original cars, only for the Carrera (only those with power of 2 mass values are "bug free" and work "properly"). This fact has very important consequences for the driving technique of Stunts cars. Usual values for the Drag parameter are similar in magnitude to those of mass, ranging from 20 to 52 for the original cars.

Additionally, Rolling resistance (ground drag) on the road is not modelled by the game, as if it were a comparatively minor effect. On grass, where rolling resistance matters more, it is modelled. See Grass Slowdown in the Car model physics article for more information.

### Grip

*Data type: **Integer***

*Address: **A4h/A5h** (simd); **CAh/CBh** (absolute)*

This is the primary handling parameter. Higher values make it possible to take corners at higher speeds without skidding, and thus raise cornering speeds as well as lower the risk of loss of control (at the relatively small cost of making controlled sliding harder, which sometimes can be an inconvenience in competition racing). A general idea of the magnitude of the parameter may be gained by attempting to drive snake lines on an asphalt track without skidding. With grip at 0100h, the car starts to skid at ~70mph. Raising to 0200h allows one to swerve without skidding almost up to 150mph. The default cars have values ranging from slightly lower than 0100h to slightly higher than 0200h.

### Surface grip modifiers

*Data type: **Integer** (x4)*

*Address: **B6h/B7h...BCh/BDh** (simd); **DCh/DDh...E2h/E3h** (absolute)*

These values are responsible for modifying the car grip according to the kind of surface it is on. The four integer values correspond to the four kinds of surface of Stunts - asphalt, dirt, ice and grass, in that order. Most cars have these set to 0100h, 00C0h, 0040h and 0080h, and thus asphalt grip = (4/3)*dirt grip = 4*ice grip = 2*grass grip. LM-002, as an off-road car, is a notable exception.

### Air grip

*Data type: **Integer***

*Address: **B4h/B5h** (simd); **DAh/DBh** (absolute)*

This value, which is set to zero on all original cars and certainly wasn't intended to be adjustable, acts as a grip modifier for airborne wheels. Raising it from zero introduces the highly unrealistic possibility of turning the car at the beginning of a jump, while the front wheels are on air and the rear ones still on the ground. "Air slides" (the mid-air spinning of car gone airborne) in particular become more pronounced, even if the car was not sliding at all before leaving the ground. Setting it at a too high value will make the car extremely sensitive to direction changes when it leaves the ground, and thus make it largely undrivable.

### Wheel coordinates

*Data type: **Triplet** of **integers** (x4)*

*Address: **{D2h...D7h}...{E4h...E9h}** (simd); **{F8h...FDh}...{10Ah...10Fh}** (absolute)*

These values set the physical position of the car wheels. The coordinates are used for the expected purposes - such as reorienting the car when moving to a different surface and verifying whether it is partly or fully over grass or water - but they also act as a bounding box for collisions against walls and track surfaces. This fact can be readily verified by noticing cars only actually crash at obstacles when the front wheels hit them. Each of the wheel coordinates is stored as a triplet of integers. The first triplet corresponds to the front/left wheel; the other three stand for the remaining ones, ordered clockwise. As usual, the first value of the triplet is the x (left/right) coordinate, and the third one the z (back/front) one. The second value, which was supposed to be the y (vertical) coordinate, has no actual effect, and is always set to zero. The coordinate values are taken relative to the middle of the car, and both the sum of x coordinates and the sum of y coordinates must be zero - not following this recommendation will result in bugs such as the car moving forward or rotating on its own. Most reasonable cars will have wheels positioned on the vertexes of a rectangle symmetrical about the centre of the car, and thus the values in the four triplets will be:
 -half_width, 0, +half_wheelbase
 +half_width, 0, +half_wheelbase
 +half_width, 0, -half_wheelbase
 -half_width, 0, -half_wheelbase
On the above, wheelbase is the distance between front and rear wheels/axles, and width is understood as wheel-to-wheel separation, all distances being measured from the centre of the wheels. Finally, the ratio between the physical coordinates for the wheels and the corresponding 3D shape coordinates is 64:1.

#### Influence of the wheelbase on handling

The wheelbase, as defined by the z coordinates of the wheels, has major effects on car behaviour. Shortening a car wheelbase will make it change direction faster, as if the moment of inertia had been reduced. As a consequence, shorter cars will:
* Have more nimble steering, a fact which not only makes them easier to drive but also allows for higher cornering speeds at the same grip levels - since less steering effort will be needed to turn the car, the risk of skidding will be significantly reduced.
* Land at shorter distances from jumps, as the car will be pointing down earlier on the flight.
* Be more sensitive to surface changes, and generally have more twitchy behaviour. A key consequence is that effects such as magic carpets become more likely.
The unique handling characteristics of Audi and Lancia are a consequence of their short length, which gives them much better handling than other slow cars and makes them more prone to random bugs.

### Dimensions for car-car collisions

*Data type: **Integer** (x4)*

*Address: **C8h/C9h...CEh/CFh** (simd); **EEh/EFh...F4h/F5h** (absolute)*

The first three integers in this set of four half-width, height and half-length values for the car. These are only used for the detection of collisions with the opponent car and other box-shaped obstacles, such as trees and slalom blocks. Unlike the main set of dimensions, these correspond roughly to the full length and width of the car as given by the car1 3D shape, and the units are the same as the ones on car1. The fourth value is a cutoff radius -- if the distance between car and obstacle is larger than the sum of the radii of both, no collision will happen. If all four values are set to zero, it becomes possible to drive straight through the opponent car, like in Trackmania for instance. Unfortunately, the AI won't realize that and will still swerve to avoid your car even if it is not necessary to do so.

## Dashboard controls

In addition to physical data, car*.res parameters are responsible for controlling the displacement of mobile elements of the dashboard, that is, bitmaps from stdb*.pvs (from Car files), speedometer and tachometer needles. Those parameters, alongside with a few related ones, are described below. Positions of moving dashboard elements (such as the shifting knob) are defined using a set of internal coordinates relative to some fixed position bitmap in stda*.pvs (from Car files). According to those, the (0,0) point corresponds to the top left pixel of the reference bitmap; thus horizontal coordinates raise when moving *rightwards* and vertical coordinates raise when moving *downwards*. An illustration of this scheme is presented on the image beside for the 48x52 pixels bitmap of Lancia's gearbox - the essentials presented apply to other instruments as well. Some of the discussions on this subsection refer to the structure of bitmap resource files (stda* and stdb*). In case of doubts about such issues, check the Car files and Resource file format articles.

### Shifting knob positions

*Data type: **Pairs** of **integers** (x6)*

*Address: **(20h/21h, 22h/23h)...(34h/35h, 36h/37h)** (simd); **(46h/47h,48h/49h)...(5Ah/5Bh,5Ch/5Dh)** (absolute)*

These parameters control where the knob will appear at each gear. The first integer of the pair sets the x coordinate, and the second one, the y coordinate. The reference frame for knob internal coordinates is the gbox bitmap at the stda* file (as the example image illustrates). When editing these values (as well as the centre position, explained just below) it is important to be realize that the distance between two gear positions and the number of direction changes on moving from one to another affects the time needed to engage the gear. Such variations have very noticeable effects on straight-line acceleration. Therefore, car tuners should be wary when modifying knob positions of an existing car so as to not alter the car performance and thus compromise pre-existing replays.

### Shift pattern centre line

*Data type: **Integer***

*Address: **1Eh/1Fh** (simd); **44h/45h** (absolute)*

This is a complementary value to the shifting knob positions. It sets at which y coordinate value the knob will switch from one column to another on the shift pattern; and thus it will most likely it is to be set halfway between the upper and lower rows of gear positions. The coordinate frame is the same of the knob positions.

### Apparent car height

*Data type: **Integer***

*Address: **D0h/D1h** (simd); **F6h/F7h** (absolute)*

This sets the apparent height from the ground on the inside (F1) view. The units are the same than those of the car1 3D shape. The parameter has no effect on the physical characteristics of the car, so you do not need to worry about crashing at roofs if you set it too high...

### Steering wheel dot movement

*Data type: **Pairs** of **Bytes** (x 1 + 30)*

*Address: **(EAh, EBh)** and **(ECh, EDh)...(126h, 127h)** (simd); **(110h,111h)** and **(112h, 113h)...(14Ch,14Dh)** (absolute)*

These pairs of bytes control the position of the steering wheel dot over its full range of movement. The first pair is the dot position with the wheel centred. As for the other 30 pairs, each one controls the dot position for a certain amount of steering applied ''both'' when turning to the right (actual coordinate value) and to the left (horizontal coordinate is mirrored relative to the central dot). This also means that displacing the centre point without adjusting the other points will separate the right side points from the left side ones... The reference frame for the internal coordinates is stda* whl2 bitmap.

### Speedometer needle movement

*Data type: **Pairs** of **Bytes**, plus one pair of **integers** and a single integer (1 pair of integers, 1 integer and 104 pairs of bytes)*

*Address: **(128h/129h, 12Ah/12Bh), 12Ch/12Dh, (12Eh/12Fh, 130h/131h)...(1FCh/1FDh, 1FEh/1FFh)** (simd); **(14Eh/14Fh,150h/151h), 152h/153h, (154h/155h)...(222h,223h)** (absolute)*

Like the other meters, the movement is controlled by pairs of values which set the position of individual points. In this case, the coordinate frame are the coordinates of stda* ins2 bitmap. The lone pair of integers starting at the beginning defines the centre point for the needle. The next integer sets the maximum number of needle positions to be displayed. This value is always less than 256, so in practice, only the first of its bytes is used and the other one is always zero. Finally, the position of the needle tip for each speed is controlled by the consecutive pairs from the next position on. Each pair of bytes covers an interval of 2.5 mph, starting from zero. Finally, in case one needs to get rid of the needle altogether (like for IMSA dashboards), the easiest solution is to set the lower bytes of the first two integers to FFh and all the pairs after the count integer zero - simply setting the count integer to zero won't work as expected.

### Digital speedometer

*Data type: **Pairs** of **bytes***

*Address: **(12Eh, 12Fh), (130h, 131h)** and **(132h, 133h)** (simd); **(154h,155h), (156h,157h)** and **(158h,159h)** (absolute)*

This is a funny one. To have a digital speedometer first one needs to have an STDB* file containing the dig0...dig9 bitmaps used for the digits (the classic solution is to clone the Corvette STDB*). Additionally, a dashboard with proper contrast must be chosen (even if the game can switch number colours automatically depending on the background, some dashboards will still look awful). Then, byte 150h must be set to 0h. That will trigger the analog needle to disappear and be replaced by the set of digits, whose coordinates are controlled in the usual fashion by the three pairs, which stand for the first, second and third digits from left to right respectively. As for the coordinates' reference frame, it is stda* ins2 bitmap, just as for the analog speedometer.

### Rev meter needle movement

*Data type: **Pairs** of **bytes**, plus one pair of **integers** and a single byte (1 pair of integers, 1 integer and 128 pairs of bytes)*

*Address: **(1FEh/1FFh, 200h/201h), 202h/203h (204h/205h)...(302h/303h)** (simd); **(224h/225h,226h/227h), 228h/229h, (22Ah/22Bh)...(328h,329h)** (absolute)*

The rev meter is completely analogous to the speedometer. The pair of integers at the beginning sets the centre point, the next integer sets the number of needle positions to be displayed and the pairs from the word that follows to define the tip positions, adding up to 128 available positions. Every pair of bytes covers 128rpm, starting from zero. Other details are just the same as those discussed for the speedometer. Again, the reference frame is provided by stda* ins2 bitmap.

## Text data

### Car information

*Data type: **Zero-terminated character strings***

*Absolute address: **32Eh - ???h***

Quoting [Lukas Loehrer](http://www.kalpen.de/luke/4dinfo.html#cs):

> *Car information displayed on the 'choose your car' screen. Use ] (5Dh) for linebreaks. The end of this block can not be given by an absolute address. Look for a byte with the value 00h. It is followed by a 4 byte long car id which is again terminated by a 00h.*

The "4 byte long car id" mentioned above is the abbreviation displayed alongside opponent name in the scoreboard. Car information is most conveniently edited via Car Blaster, which has a specific WYSIWYG interface for dealing with it.

### Scoreboard car name

*Data type: **Zero-terminated character string***

*Absolute address: **???h - EOF***

The final bytes of the file (after the terminating 00h mentioned above) make up the scoreboard designation of the car. It can be edited via Car Blaster like the other text pieces as well.

## Miscellaneous

### File header

*Address: **00h - 25h***

The initial bytes of CAR*.RES files consist of a file header, structured just like the ones found in uncompressed graphic files and elsewhere. Essentially, it stores file length, text IDs for different sections of the file and the associated byte offsets. Since that data is actually used by the engine to parse the car data, there is no reason to modify it unless one is hand-editing text data for the car and intends to modify string length - and even in that case, using Car Blaster and allowing it to perform any necessary adjustment is the recommended option.

### Unknown parameters

Here is a list of the values on CAR*.RES which have unknown function. After somewhat extensive testing, they displayed no visible effect on the behaviour of the car in saved replays. Most of them are equal for all cars.

* 027h - single byte next to the number of gears, probably just a padding zero.
* 034h/035h - strategically located next to the gear ratios. Unused space for a never-implemented reverse gear?
* 042h/043h - looks like one of the other coordinates of the shifting knob, but has no function.
* 0C8h/0C9h - set to 00B5h for all cars.
* 0CCh/0CDh - set to 00EEh for all cars.
* 0CEh/0CFh - set to 00B8h for all cars.
* 0D0h/0D1h - set to zero for all cars.
* 0D2h/0D3h, 0D4h/0D5h, 0D6h/0D7h, 0D8h/0D9h - these form a suspicious-looking series with values 0020h, 0010h, 00C0h and 0008h, equal for all cars.
* 0E4h/0E5h - set to zero for all cars.
* 0E6h/0E7h, 0E8h/0E9h, 0EAh/0EBh, 0ECh/0EDh - another series of values, this time 0020h, 00C0h, 0080h and 0040h. The resemblance to the default values of the surface grip modifiers, which are located near these positions, is uncanny: were they intended to refer to a second set of tyres?
* 32Ah/32Bh/32Ch/32Dh - in memory, these positions are used by the game engine for calculations. They aren't parameters at all -- their values in the files are ignored.

# Car behaviour (car*.res)

All adjustable aspects of a car behaviour are stored in car*.res, a miscellaneous data resource file. It contains both physics data, such as power curves, and visual parameters, like car height in cockpit view or speedometer behaviour. Also, the file contains the text data displayed at the car selection screen. Data in car*.res may be read and modified by any hex editor, or more conveniently by a specially-designed graphical editor (see the Tools section ahead).

The structure of car*.res actually consists of four resources:
- **simd**, which contains all the numerical parameters;
- **edes**, a text resource containing the description displayed in the car selection screen;
- **gsna**, the 4-letter scoreboard abbreviation stored as a text resource; and
- **gnam**, the scoreboard car name.

Only the **simd** chunk is of fixed length (306 bytes) while the other three are variable-length ASCIIZ strings. While the order in which these chunk indices and their corresponding file sections is irrelevant to Stunts, all of the original cars have come with the **simd** chunk content first, resulting in the positions of car parameters appearing absolute (that is, relative to the beginning of the file). However, new cars or cars modified with new tools may not follow this order. More info in the Tools section.

In the following table, "simd offset" refers to the actual position of the parameter within the **simd** chunk, that is, the way Stunts interprets the file. "Absolute offset" refers to the position relative to the beginning of the file in the cases in which the **simd** chunk contents have been placed first:

| simd offset | Absolute offset | Function |
|---|---|---|
| 000h | 26h | Number of gears |
| 001h | 27h | *"Purple" byte |
| 002h | 28h/29h | Car mass |
| 004h | 2Ah/2Bh | Braking effectiveness |
| 006h | 2Ch/2Dh | Idle rpm |
| 008h | 2Eh/2Fh | Downshift rpm (auto transm.) |
| 00Ah | 30h/31h | Upshift rpm (auto transm.) |
| 00Ch | 32h/33h | Maximum rpm |
| 00Eh | 34h/35h | *"Yellow" word |
| 010h | 36h-41h | Gear ratios |
| 01Ch | 42h/43h | *Unknown. Horizontal gear knob tabulation? |
| 01Eh | 44h/45h | Shift pattern centre line |
| 020h | 46h-5Ch | Shifting knob positions |
| 038h | 5Eh/5Fh | Aerodynamic resistance |
| 03Ah | 60h | Idle rpm torque |
| 03Bh | 61h-C7h | Torque curve |
| 0A2h | C8h/C9h | *"Blue" word |
| 0A4h | CAh/CBh | Grip |
| 0A6h | CCh-D9h | *"Red" words (7 of them. Defaults: 238, 184, 0, 32, 16, 12, 8) |
| 0B4h | DAh/DBh | Air grip |
| 0B6h | DCh-E3h | Surface grip modifiers |
| 0BEh | E4h-EDh | *"Orange" words (5 of them. Defaults: 0, 32, 192, 128, 64). More surf. grip? |
| 0C8h | EEh-F5h | Dimensions for car-car collisions |
| 0D0h | F6h/F7h | Car height (from inside view) |
| 0D2h | F8h-10Fh | Wheel coordinates / external dimensions |
| 0EAh | 110h-14Dh | Steering wheel dot movement |
| 128h | 14Eh-223h | Speedometer needle movement |
| 12Eh | 154h-159h | Digital speedometer |
| 1FEh | 224h-329h | Rev. meter needle movement |
|  | 32Eh-EOF | Text data (car selection screen and high score table) |

Fields marked with a star (*) are either not used by the game or its impact on behaviour is unknown. The colour names have been given by someone for easier reference. Only red #5 has been used on a mod to change the instrument needle colour.