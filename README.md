# axo

Some top level patches on the axoloti code to get a clean build on my system.

# rates

* k-rate = 3 KHz
* s-rate = 48 KHz (x16)

# connector coloring

* Yellow = bool32 at k-rate
* Blue = frac32 at k-rate
* Green = int32 at k-rate
* Red = frac32 at s-rate (16 x buffer)

# variable formats

frac32 : -0x08000000 to 0x07FFFFFF corresponds to -64.0 .. 64.0

# variable prefixes

* inlet_ : input
* outlet_ : outlet
* param_ : parameter
* disp_ : display
