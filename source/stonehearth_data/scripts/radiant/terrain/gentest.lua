-- squintfrac.lua
-- This is a set of support functions for generating a fractal noise pattern
-- fractal noise is a wonderful thing if you want more natural looking
-- "random" worlds.

math.randomseed(os.time())

FractalHeight={}
FractalMaxHeight=0

-- This function takes boundaries for the noise, and the number of octaves to generate
-- it is not particularly clever - the size in each direction should be a multiple of
-- 2 to the power of (the number of octaves - 1) + 1
-- (ie, ocataves = 3, width = N*(2^2)+1 = 5, 9, 13, ... etc )
-- It doesn't break if you get this wrong, it just pads the end with low values.
function FractalGenerate(xsize, ysize, octaves)
  -- begin by initialising everywhere to 0
  for x=1,xsize,1 do
    FractalHeight[x]={}
    for y = 1, ysize, 1 do
      FractalHeight[x][y]=0
  end
  end

  -- add each octave to the fractal
  FractalMaxHeight = 0
  FractalAddOctave(xsize, ysize, octaves, CutoffHeight)

end

-- recursively adds octaves to the fractal
function FractalAddOctave(xsize, ysize, octave)
  -- add this octave
  local step = 2^(octave - 1)
  local scale = step
  FractalMaxHeight = FractalMaxHeight + scale
  for x = 1, xsize, step do
    for y = 1, ysize, step do
    -- Add random value at this point
    FractalHeight[x][y] = FractalHeight[x][y] + math.random(0,scale)
    -- if we aren't on the last octave, interpolate intermediate values
    if step > 1 then
      if x > 1 then
        FractalHeight[x - step / 2][y] = (FractalHeight[x - step][y] + FractalHeight[x][y]) / 2
    end
        if y > 1 then
      FractalHeight[x][y - step / 2] = (FractalHeight[x][y - step] + FractalHeight[x][y]) / 2
    end
    if x > 1 and y > 1 then
      FractalHeight[x - step / 2][y - step / 2] = (FractalHeight[x - step / 2][y - step] + FractalHeight[x - step / 2][y]) / 2
    end
    end
  end
  end

  -- add the next octave
  if octave > 1 then
    FractalAddOctave(xsize, ysize, octave - 1)
  end
end


function noise( x,  y,  z)
    local X = math.floor(x % 255)
    local Y = math.floor(y % 255)
    local Z = math.floor(z % 255)
    x = x - math.floor(x)
    y = y - math.floor(y)
    z = z - math.floor(z)
    local u = fade(x)
    local v = fade(y)
    local w = fade(z)
 
    A   = p[X  ]+Y
    AA  = p[A]+Z
    AB  = p[A+1]+Z
    B   = p[X+1]+Y
    BA  = p[B]+Z
    BB  = p[B+1]+Z
 
    return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ),
                                 grad(p[BA  ], x-1, y  , z   )),
                         lerp(u, grad(p[AB  ], x  , y-1, z   ),
                                 grad(p[BB  ], x-1, y-1, z   ))),
                 lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ),  
                                 grad(p[BA+1], x-1, y  , z-1 )),
                         lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                 grad(p[BB+1], x-1, y-1, z-1 )))
    )
end
 
 
function fade (t)
    return t * t * t * (t * (t * 6 - 15) + 10)
end
 
 
function lerp(t,a,b)
    return a + t * (b - a)
end
 
 
function grad(hash,x,y,z)
    local h = hash % 16
    local u
    local v
   
    if (h<8) then u = x else u = y end
    if (h<4) then v = y elseif (h==12 or h==14) then v=x else v=z end
    local r
   
    if ((h%2) == 0) then r=u else r=-u end
    if ((h%4) == 0) then r=r+v else r=r-v end
    return r
end
 
 
p = {}
local permutation = { 151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
}
 
for i=0,255 do
    p[i] = permutation[i+1]
    p[256+i] = permutation[i+1]
end

-- http://www.dynaset.org/dogusanh/download/luacairo.html
if false then
   local cairo = require 'lcairo'
else
   local cairo
end
require 'unclasslib'

local Teragen = class()
function Teragen:__init(options)
   if options.size then
      self:create(options)
   end
end

function Teragen:create(options)
  self._size = options.size or 1024
  self._octaves = options.octaves or 5
  self._wavelength = options.wavelength or 32
  self._offset_x = options.offset[1]
  self._offset_y = options.offset[2]
  FractalGenerate(self._size, self._size, self._octaves)
end

function Teragen:_sample(x, y)
  value = 0
  if false then
    for i = 1, self._octaves do
      local wavelength = i + self._wavelength
      local sample_x = (x + self._offset_x) * wavelength / 256
      local sample_y = (y + self._offset_y) * wavelength / 256
      local sample = noise(sample_x, sample_y, 0) + 0.5
      value = value + (sample / self._octaves)
    end
  else
    value = FractalHeight[x + 1][y + 1] / FractalMaxHeight
    local levels = 6
    value = math.floor(value * levels + 0.5) / levels
  end
  return value
end

function Teragen:output(level)
   local point_size = 8
   local image_size = self._size * point_size
   local cs = cairo.image_surface_create(cairo.FORMAT_RGB24, image_size, image_size)
   local cr = cairo.create(cs)
   for y = 0, self._size - 1 do
      for x = 0, self._size - 1 do
         local h = self:_sample(x,  y)
         cairo.set_source_rgb(cr, h, h, h)
         cairo.rectangle(cr, x * point_size, y * point_size, point_size, point_size)
         cairo.fill(cr)
      end
   end

   local filename = string.format('teragen_level_%d.png', level)
   cairo.surface_write_to_png(cs, filename)
end

if false then
   local tg = Teragen({
         size = 128,
         octaves = 5,
         offset = {64, 153}
      })
   tg:output(0)
end
--[[
print(cairo.version_string())

w = 320
h = 240
outfile = 'cairo_test1.png'

cs = cairo.image_surface_create(cairo.FORMAT_RGB24, w, h)

cr = cairo.create(cs)

cairo.set_source_rgb(cr, 1, 1, 0.5)
cairo.paint(cr)

cairo.set_source_rgb(cr, 0, 0, 0)
cairo.set_line_width(cr, 5)
cairo.set_line_cap(cr, cairo.LINE_CAP_ROUND)

cairo.move_to(cr, w/4, h/4)
cairo.line_to(cr, 3*w/4, 3*h/4)
cairo.stroke(cr)

cairo.move_to(cr, 3*w/4, h/4)
cairo.line_to(cr, w/4, 3*h/4)
cairo.stroke(cr)

cairo.surface_write_to_png(cs, outfile)
]]
