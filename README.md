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


# display types (disp_)
name | type 
 --- | --- 
 vscale | 
 frac4byte.vbar | 
 frac32.vbar | 
 frac32.u.dial | 
 frac32.s.chart | 
 int32.bar16 | 
 frac4ubyte.vbar.db | 
 int32.bar32 | 
 frac32.vu | 
 int8array128.vbar | 
 frac32.vbar.db | 
 frac32.u.chart | 
 frac32.s.dial | 
 note.label | 
 bool32 | 
 frac4ubyte.vbar | 
 uint8array128.vbar | 
 int32.hexlabel | 
 int32.label | 

# outlet types (outlet_)
 name | type 
  --- | --- 
 bool32.pulse | 
 bool32 | 
 int32 | 
 int32.positive | 
 int32.bipolar | 
 frac32.positive | 
 frac32buffer | 
 frac32buffer.positive | 
 frac32buffer.bipolar | 
 frac32.bipolar | 
 charptr32 | 
 frac32 | 

# attribute types (attrib_)
 name | type 
  --- | --- 
 combo | 
 unused! | 
 int | 
 text | 
 table | 
 spinner | 
 objref | 
 file | 
 
 # parameter types (param_)
 name | type 
  --- | --- 
 bin32 | 
 frac32.u.map | 
 int32.mini | 
 frac32.s.map.kpitch | 
 frac32.s.map | 
 bool32.tgl | 
 frac32.u.map.squaregain | 
 bool32.mom | 
 frac32.u.map.filterq | 
 frac32.u.map.kdecaytime.reverse | 
 frac32.u.mapvsl | 
 frac32.u.map.freq | 
 int32.hradio | 
 frac32.s.map.klineartime.exp | 
 frac32.u.map.kdecaytime | 
 frac32.s.map.pitch | 
 int2x16 | 
 frac32.s.map.kdecaytime.exp | 
 frac32.u.map.gain | 
 bin12 | 
 frac32.u.map.klineartime.reverse | 
 frac32.s.mapvsl | 
 int32 | 
 int32.vradio | 
 bin16 | 
 frac32.s.map.klineartime.exp2 | 
 frac32.s.map.lfopitch | 
 frac32.u.map.ratio | 
 frac32.u.map.gain16 | 
 frac32.s.map.ratio | 
 
 # inlet types (inlet_)
 name | type 
  --- | --- 
 bool32.rising | 
 int32.bipolar | 
 charptr32 | 
 frac32buffer.bipolar | 
 frac32.positive | 
 bool32 | 
 frac32 | 
 int32 | 
 int32.positive | 
 frac32.bipolar | 
 frac32buffer.positive | 
 bool32.risingfalling | 
 frac32buffer | 


