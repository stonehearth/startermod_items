
function radiant.keys(t)
   local n, keys = 1, {}
   for k, _ in pairs(t) do
      keys[n] = k
      n = n + 1
   end
   return keys
end

function radiant.size(t)
   local c = 0
   for _, _ in pairs(t) do
      c = c + 1
   end
   return c
end

function radiant.alloc_region3()
   if _radiant.client then
      return _radiant.client.alloc_region3()
   end
   return _radiant.sim.alloc_region3()
end

function radiant.copy_table(t)
   local copy = {}
   for k, v in pairs(t) do
      copy[k] = v
   end
   return copy
end
