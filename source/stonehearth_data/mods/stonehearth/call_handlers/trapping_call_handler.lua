local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local TrappingCallHandler = class()

-- runs on the client!!
function TrappingCallHandler:choose_trapping_grounds_location(session, response)
   stonehearth.selection:select_designation_region()
      :set_min_size(10)
      :set_max_size(50)
      :require_unblocked(false)
      :use_designation_marquee(Color4(122, 40, 0, 255))
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
            _radiant.call('stonehearth:create_trapping_grounds', box.min, size)
               :done(
                  function(r)
                     response:resolve({ trapping_grounds = r.trapping_grounds })
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

function TrappingCallHandler:create_trapping_grounds(session, response, location, size)
   local trapping_grounds = stonehearth.trapping:create_trapping_grounds(session.player_id, location, size)
   return { trapping_grounds = trapping_grounds }
end

return TrappingCallHandler
