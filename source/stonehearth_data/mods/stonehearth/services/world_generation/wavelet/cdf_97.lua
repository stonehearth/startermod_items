local CDF_97 = {}
-- Cohen-Daubechies-Feauveau 9/7 wavelet
-- excellent cutoff and orthogonality properties
-- lifting implementation so we can use symmetric boundary extension
-- n < 8 will have increasing boundary condition effects

local P1 = -1.586134342
local U1 = -0.05298011854
local P2 =  0.8829110762
local U2 =  0.4435068522
local S  = 1/1.149604398

local temp = {}

function CDF_97.DWT_1D(src, length)
   assert(length % 2 == 0)
   assert(length >= 4)

   CDF_97._predict(src, length, P1)  -- Predict 1
   CDF_97._update(src, length, U1)   -- Update 1
   CDF_97._predict(src, length, P2)  -- Predict 2
   CDF_97._update(src, length, U2)   -- Update 2
   CDF_97._scale(src, length, S)     -- Scale
   CDF_97._deinterleave(src, length) -- consolidate frequency components
end

function CDF_97.IDWT_1D(src, length)
   assert(length % 2 == 0)
   assert(length >= 4)

   CDF_97._interleave(src, length)   -- interleave frequency components
   CDF_97._scale(src, length, 1/S)   -- Invert Scale
   CDF_97._update(src, length, -U2)  -- Invert Update 2
   CDF_97._predict(src, length, -P2) -- Invert Predict 2
   CDF_97._update(src, length, -U1)  -- Invert Update 1
   CDF_97._predict(src, length, -P1) -- Invert Predict 1
end

function CDF_97._predict(x, n, a)
   local i

   for i=2, n-2, 2 do
      x[i] = x[i] + a*(x[i-1]+x[i+1])
   end

   x[n] = x[n] + 2*a*x[n-1]
end

function CDF_97._update(x, n, a)
   local i

   x[1] = x[1] + 2*a*x[2];

   for i=3, n, 2 do
      x[i] = x[i] + a*(x[i-1]+x[i+1])
   end
end

function CDF_97._scale(x, n, a)
   local i
   local one_over_a = 1/a
   
   for i=1, n, 2 do
      x[i] = x[i] * one_over_a
      x[i+1] = x[i+1] * a
   end
end

function CDF_97._deinterleave(x, n)
   local i
   local w = n*0.5

   for i=1, w do
      temp[i] = x[i*2-1]
      temp[i+w] = x[i*2]
   end

   for i=1, n do
      x[i] = temp[i]
   end
end

function CDF_97._interleave(x, n)
   local i
   local w = n*0.5

   for i=1, w do
      temp[i*2-1] = x[i]
      temp[i*2] = x[i+w]
   end

   for i=1, n do
      x[i] = temp[i]
   end
end

----------

-- test for perfect reconstruction
function CDF_97._test()
   local i, j
   local length = 32
   local x = {}
   local y = {}
   local DELTA = 1e-13

   -- Makes a fancy cubic signal
   for i=1, length do
      j = i-1
      x[i] = 5 + j + 0.4*j^2 - 0.02*j^3
   end

   for i=1, length do
      y[i] = x[i]
   end

   CDF_97.DWT_1D(y, length)
   CDF_97.IDWT_1D(y, length)

   local diff
   for i=1, length do
      diff = math.abs(y[i]-x[i])
      assert(diff < DELTA)
   end
end

return CDF_97

