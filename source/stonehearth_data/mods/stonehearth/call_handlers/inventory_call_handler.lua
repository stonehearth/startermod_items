local Color4 = _radiant.csg.Color4
local InventoryCallHandler = class()

-- runs on the client!!
function InventoryCallHandler:choose_stockpile_location(session, response)
   stonehearth.selection:select_designation_region()
      :set_max_size(20)
      :use_designation_marquee(Color4(0, 153, 255, 255))
      :set_cursor('stonehearth:cursors:zone_stockpile')
      :done(function(selector, box)
            local size = {
               x = box.max.x - box.min.x,
               y = box.max.z - box.min.z,
            }
            _radiant.call('stonehearth:create_stockpile', box.min, size)
                     :done(function(r)
                           response:resolve({ stockpile = r.stockpile })
                        end)
                     :always(function()
                           selector:destroy()
                        end)         
         end)
      :fail(function(selector)
            selector:destroy()
            response:reject('no region')            
         end)
      :go()
end

function InventoryCallHandler:create_stockpile(session, response, location, size)
   local inventory = stonehearth.inventory:get_inventory(session.player_id)
   local stockpile = inventory:create_stockpile(location, size)
   return { stockpile = stockpile }
end

function InventoryCallHandler:get_inventory(session, response, location, size)
   local inventory = stonehearth.inventory:get_inventory(session.player_id)
   return { inventory = inventory }
end

return InventoryCallHandler
