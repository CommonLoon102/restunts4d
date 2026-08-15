# Resource file format

Stunts game data are stored in a common container format. Files of this format can hold multiple *resources*, blocks of data that may be of various types - text, bitmaps, sounds, etc. Resources are uniquely identified within each file solely by a 4-byte name. A resource container file can be encapsulated by compression.

## File names

Different file name extensions are used to indicate the content of the files. Compressed files have a leading `P` in their extensions. Whether the game prefers raw or compressed files varies based on file type.

| File contents | Raw | Compressed | Preferred |
| --- | --- | --- | --- |
| Text and misc. settings | RES | PRE | Raw |
| VGA Bitmap images | VSH | PVS | Compressed |
| EGA Bitmap images | ESH | PES | Compressed |
| 3d shapes | 3SH | P3S | Compressed |
| Music tracks | KMS | PKM | Raw |
| Instrument samples | VCE | PVC | Raw |
| Sound effects | SFX | PSF | Raw |

## Structure

The resource file header consists of two integer fields denoting the total length of the file and the number of resources contained. The following table of contents has a list of ids and a corresponding list of offsets into the remaining data section.

```c
// Header
uint32 fileLength
uint16 numResources

// Table of contents
char   ids[numResources][4]
uint32 offsets[numResources]

// Resource data
char   data[]
```

## Types

Resource type can be determined by looking at the resource's id string and/or the source file name.

### Plain text

Text resources are null-terminated [C strings](http://en.wikipedia.org/wiki/C_string) found in `RES`/`PRE` files. Strings can contain non-alphanumeric codes used by the game to achieve certain effects. Most prominent is the `]` (right square bracket) used to represent [newline](http://en.wikipedia.org/wiki/Newline) in multi-line text.

### VGA Bitmap images

VGA bitmaps are images with 8-bit color depth using a fixed palette in VGA mode. Special bitmaps using the naming scheme `!cg_` and `!eg_` provide color mapping for graphics modes with fewer available colors.

```c
uint16 width
uint16 height
uint16 centreX
uint16 centreY
uint16 positionX
uint16 positionY
uint8  unknown[4]

uint8  image[width * height]
```

Width and height correspond, of course, to the image dimensions. `centreX` and `centreY` refer to the coordinates of the axial centre for moving images, as is the case of the gear knob and the steering dot. These coordinates set the point in the image that will move along the path established for it to go along.

Parts of this structure are not fully understood. `unknown` seems to affect how image pixels are organized in compressed resource files, likely to gain more effective [run-length compression](http://en.wikipedia.org/wiki/Run-length_encoding). Modifying these values randomly for all images in a car does not appear to have any effect, which suggests this might not have importance for `PVS`/`VSH` (= VGA graphics) files. The only exceptions are the 5th and 6th bit of the 3rd element: the former one determines whether the image is stored in transposed (column-major) order, the second whether it's in interlaced order.

Due to the internal resolution of Stunts (which uses [VGA Mode 13h](http://en.wikipedia.org/wiki/Mode_13h)), non-square pixels are used for screen display. Aspect ratio corrections should be accounted for to avoid unwanted results.

On VGA settings, Stunts runs at an internal resolution of 320x200, typical of VGA Mode 13h. 320x200 is a non-standard aspect ratio (horizontal:vertical ratio 8:5, as opposed to the regular 4:3), and thus images are effectively stretched vertically by a factor of 6:5 in order to fit the 4:3 ratio before being displayed. DOSBox users should be aware that unless the aspect=true setting under the [render] section of dosbox.conf is present the adjustment may not be done, leading to inconsistent length:width ratios for 3D models and vertically shorter dimensions for both 3D objects and bitmaps. Modders should also account for aspect ratio adjustments, since bitmaps and 3D shapes should be designed taking in account the stretch - that is, vertical dimensions be input reduced by a factor of 5:6 in relation to the desired results. stressed applies the 6:5 correction on its preview window for 3D models, but not for bitmaps.

The VGA palette itself is encoded as the `!pal` bitmap resource of `SDMAIN.PVS`. The pixel data of that resource is interpreted as 256 triplets of [6-bit RGB](https://moddingwiki.shikadi.net/wiki/VGA_Palette).

### EGA Bitmap images

EGA bitmaps are only used in a handful of places, notably for the icons of the resource editor. They were present more extensively in earlier DSI titles like *Test Drive* and *Grand Prix Circuit*. The format is similar to the VGA ones, except:

- The image data is stored as 4 vertically stacked 1-bpp images, representing the color channels.
- The `unknown[4]` determines how such channels are mapped into the EGA palette color bits. The most common value is `{ 0x1, 0x2, 0x4, 0x8 }`, where each channel naturally maps into the corresponding bit ([Planar](https://moddingwiki.shikadi.net/wiki/Raw_EGA_data)). However, there are many exceptions. Also, some channels might be stored in transposed order (column first): the high nibble of the third element (don't ask) determines which channels are transposed.
- The resource is not guaranteed to contain all the four channels entirely. E.g. if the last channel has an `unknown` of `0`, it may be omitted.
- The image width is given in bytes. The pixel width is obtained by multiplying such value by 8.

### 3d shapes

Shapes are composed of a vertex list and a list of primitives. Primitives are basic shapes that can have different types, such as polygons or wheels. Depending on the type it contains a number of indices into the vertex list needed to draw the shape, color variations and some rendering hints. Stunts' coordinate system uses 16-bit signed integers, making the resolution relative to the scale of the model. Shapes presented in the car selection screen are more detailed than in-game shapes, thus requiring larger scaling. Experiment has shown that 1 foot in-game corresponds to 5 units, making each track tile roughly 205 feet (or 62.5 meters) wide.

Due to the counter header fields being stored in single bytes, the total amount of vertices and primitives are limited to 255 per shape.

#### Main structure

```c
uint8  numVertices
uint8  numPrimitives
uint8  numPaintJobs
uint8  reserved // Always == 0

VERTEX vertices[numVertices]
uint32 cullFront[numPrimitives]
uint32 cullBack[numPrimitives]
PRIMITIVE primitives[numPrimitives]

uint8  terminate[...] // 1-3 NULL-bytes for data alignment
```

#### Additional structures

```c
struct VERTEX {
  int16 x
  int16 y
  int16 z
}

struct PRIMITIVE {
  uint8 type
  uint8 flags
  uint8 materials[numPaintJobs]
  uint8 indices[...] // Size depends on type.
}
```

#### Primitive types

| Primitive type | Vertex indices needed | Description |
| --- | --- | --- |
| 1 | 1 | Particle, 1 pixel |
| 2 | 2 | Line segment, 1 pixel width |
| 3–10 | 3–10 | Polygon, *n* sides |
| 11 | 2 | Sphere (center + ~(radius / 2) |
| 12 | 6 | Wheel |
| * | 0 | Ignored |

#### Primitive flags

| Flag | Name |
| --- | --- |
| `0x01` | Two-sided |
| `0x02` | Z-Bias |
| * | Ignored |

#### Culling data masks

| Flag | Description |
| --- | --- |
| `0xFFFE0000` | Horizontal angle ranges, positive direction |
| `0x0001FFFC` | Horizontal angle ranges, negative direction |
| `0x00000001` | High vertical angle flag, positive direction |
| `0x00000002` | High vertical angle flag, negative direction |

For car body and track element shapes the first eight vertices are reserved for the corner vertices of the shape's bound box, used to cull entire shapes located outside the view frustum.

The first three vertices in a wheel primitive mark the center and the boundary points along the y and z axes of the circle facing the inside of the car. The last three vertices do the same, in the same order, for the circle facing the outside.

Wheel transformations are performed on fixed vertex positions. Since the first eight vertices of `car[0-2]` shapes are occupied by the bound box, vertices 9-14 and 15-20 are front wheels. Misplaced wheel vertices will lead to corrupted shape rendering.

The two vertices of a sphere primitive are the center of the sphere and a point on its boundary (only the radius matters, not the specific position on the boundary).

The `flags` in the `PRIMITIVE` structure are used for two-sided polygons (disable [back-face culling](http://en.wikipedia.org/wiki/Back-face_culling)) and to enable Z-bias in order to resolve [Z-fighting](http://en.wikipedia.org/wiki/Z-fighting).

Materials are indices into a permanent, internal game structure and are assigned per-primitive. Cars can have multiple paint jobs, and every primitive in the shape must set a material for each color scheme.

Each primitive has two 32-bit culling data fields to determine visibility. A culling data field holds angular visibility in positive and negative direction using 15 bits each. One bit corresponds to 1/15th of a full circle and the bits can be set in any combination. The two least significant bits are used as flags for visibility at near-vertical viewing angles.

A shape resource must be terminated with NULL-bytes. The number of bytes needed is not known, but padding three NULL-bytes has proven to work regardless of resource size or byte alignment. Failing to meet this requirement will lead to obscure rendering artifacts.

As for bitmaps, aspect ratio issues in Stunts should be considered when setting Y coordinates of 3D models.

### Car parameters

Numerical `car*.res` parameters are stored in a 776-byte resource identified as *simd*. Most data in `simd` is stored as signed 16-bit integers. Noteworthy exceptions include number of gears, gear ratios, torque curve data and bitmap coordinates for dashboard instruments, which are unsigned 8-bit integers. Organization and function of the parameters are outside the scope of this article; the reader should consult the main article on the topic for further information.

### Opponent parameters

The `opp?.res` files contain numerical data resources which control opponent behaviour and victory/defeat animations. The contents and function of these resources are better understood within the context of the main article.
