local RadiantIPoint3 = _radiant.math3d.RadiantIPoint3
local CreateStockpile = class()

function CreateStockpile:handle_request(query, postdata)
   local inventory = radiant.mods.require('/stonehearth_inventory').get_inventory(postdata.faction)
   local size = {
      postdata.p1.x - postdata.p0.x + 1,
      postdata.p1.z - postdata.p0.z + 1,
   }
   inventory:create_stockpile(postdata.p0, size) 
   return {}
end

return CreateStockpile
