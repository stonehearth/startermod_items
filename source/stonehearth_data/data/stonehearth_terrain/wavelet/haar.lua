local Haar = class()

-- Haar wavelet coefficients
-- Basically just sum and difference while preserving total energy
-- High leakage across spectral bands, but simple and fast
local H1 =  0.707106781186547
local H2 =  0.707106781186547
   
local G1 =  0.707106781186547
local G2 = -0.707106781186547

function Haar:__init()
end

function Haar:DWT_1D(dst, src, length)
   local i, n, A, B
   n = length/2

   assert(length >= 2)

   for i=1, n, 1 do
      A = src[2*i-1]
      B = src[2*i]

      dst[i]   = A*H2 + B*H1 -- low frequency
      dst[n+i] = A*G2 + B*G1 -- high frequency
   end
end

function Haar:IDWT_1D(dst, src, length)
   local i, n, A, B
   n = length/2

   assert(length >= 2)

   for i=1, n, 1 do
      A = src[i]
      B = src[n+i]

      dst[2*i-1] = A*H2 + B*G2
      dst[2*i]   = A*H1 + B*G1
   end
end

return Haar

