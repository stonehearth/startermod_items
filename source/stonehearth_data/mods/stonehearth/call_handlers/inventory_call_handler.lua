local Color4 = _radiant.csg.Color4
local InventoryCallHandler = class()

-- runs on the client!!
function InventoryCallHandler:choose_stockpile_location(session, response)
   stonehearth.selection:select_xz_region()
      :restrict_to_standable_terrain()
      :use_designation_marquee(Color4(0, 153, 255, 255))
      :set_cursor('stonehearth:cursors:designate_zone')
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

-- runs on the server!
function InventoryCallHandler:get_placable_items(session, response)
   local inventory = stonehearth.inventory:get_inventory(session.player_id)
   if not inventory then
      response:reject('could not find inventory for player ' .. session.player_id)
      return
   end
   local tracker = inventory:add_item_tracker('stonehearth:placable_items_view_tracker')
   return { tracker = tracker }
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

function InventoryCallHandler:get_entities_in_explored_region(session, response)
   local entities = stonehearth.terrain:get_entities_in_explored_region(session.faction)
   response:resolve({ entities = entities })
end

function InventoryCallHandler:get_talismans_in_explored_region(session, response)
   local talisman_filter_fn = function(entity)
      if entity:get_component('stonehearth:promotion_talisman') then
         return true
      else 
         return false
      end
   end

   local entities = stonehearth.terrain:get_entities_in_explored_region(session.faction, talisman_filter_fn)

   -- build a map of uris
   local available_professions = {}

   for id, entity in pairs(entities) do
      local talisman_component = entity:get_component('stonehearth:promotion_talisman')
      local profession = talisman_component:get_profession()
      available_professions[id] = profession
   end

   return({ 
      entities = entities,
      available_professions = available_professions
   })
end

return InventoryCallHandler
