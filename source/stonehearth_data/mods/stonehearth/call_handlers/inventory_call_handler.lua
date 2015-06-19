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

function InventoryCallHandler:get_talismans_in_explored_region(session, response)
   local get_talismans_from_container_fn = function(container)
      local result = {}
      local storage = container:get_component('stonehearth:storage')
      if not storage then
         return result
      end

      for id, item in pairs(storage:get_items()) do
         if item:get_component('stonehearth:promotion_talisman') then
            result[id] = item
         end
      end

      return result
   end

   local talisman_filter_fn = function(entity)
      if entity:get_component('stonehearth:promotion_talisman') then
         return true
      elseif entity:get_component('stonehearth:backpack') then 
         return next(get_talismans_from_container_fn(entity)) ~= nil
      else 
         return false
      end
   end



   local entities = stonehearth.terrain:get_entities_in_explored_region(session.player_id, talisman_filter_fn)

   -- build a map of uris
   local available_jobs = {}

   for id, entity in pairs(entities) do
      local talisman_component = entity:get_component('stonehearth:promotion_talisman')

      if not talisman_component then
         local talismans = get_talismans_from_container_fn(entity)
         for id, item in pairs(talismans) do
            talisman_component = item:get_component('stonehearth:promotion_talisman')
            local job = talisman_component:get_job()
            available_jobs[id] = job
         end
      else
         local job = talisman_component:get_job()
         available_jobs[id] = job
      end
   end

   return({ 
      entities = entities,
      available_jobs = available_jobs
   })
end

return InventoryCallHandler
