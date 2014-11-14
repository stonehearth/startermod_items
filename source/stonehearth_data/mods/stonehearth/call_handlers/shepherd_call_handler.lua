local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local ShepherdCallHandler = class()

-- Runs on the client!
function ShepherdCallHandler:choose_pasture_location(session, response)
   stonehearth.selection:select_designation_region()
      :require_unblocked(false)
      :use_designation_marquee(Color4(56, 80, 0, 255))
      :set_can_contain_entity_filter(function(entity)
            -- avoid other designations.            
            if radiant.entities.get_entity_data(entity, 'stonehearth:designation') then
               return false
            end
            return true            
         end)
      :set_cursor('stonehearth:cursors:zone_trapping_grounds')
      :done(
         function(selector, box)
            local size = {
               x = box.max.x - box.min.x,
               z = box.max.z - box.min.z,
            }
            _radiant.call('stonehearth:create_pasture', box.min, size)
               :done(
                  function(r)
                     response:resolve({ pasture = r.pasture })
                  end
               )
               :always(
                  function()
                     selector:destroy()
                  end
               )
         end
      )
      :fail(
         function(selector)
            selector:destroy()
            response:reject('no region')            
         end
      )
      :go()
end

function ShepherdCallHandler:create_pasture(session, response, location, size)
   return { pasture = nil }
end

return ShepherdCallHandler