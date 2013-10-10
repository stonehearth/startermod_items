local Haar = {}

-- Haar wavelet coefficients
-- Basically just sum and difference while preserving total energy
-- High leakage across spectral bands, but simple and fast
local H1 =  0.707106781186547
local H2 =  0.707106781186547
   
local G1 =  0.707106781186547
local G2 = -0.707106781186547

function Haar.DWT_1D(src, length)
   local i, n, A, B
   local temp = {}

   n = length/2

   assert(length >= 2)

   for i=1, n do
      A = src[2*i-1]
      B = src[2*i]

      temp[i]   = A*H2 + B*H1 -- low frequency
      temp[n+i] = A*G2 + B*G1 -- high frequency
   end

   for i=1, length do
      src[i] = temp[i]
   end
end

function Haar.IDWT_1D(src, length)
   local i, n, A, B
   local temp = {}

   n = length/2

   assert(length >= 2)

   for i=1, n do
      A = src[i]
      B = src[n+i]

      temp[2*i-1] = A*H2 + B*G2
      temp[2*i]   = A*H1 + B*G1
   end

   for i=1, length do
      src[i] = temp[i]
   end
end

return Haar

