local Point2 = _radiant.csg.Point2
local Color4 = _radiant.csg.Color4

local TrappingCallHandler = class()

-- runs on the client!!
function TrappingCallHandler:choose_trapping_grounds_location(session, response)
   stonehearth.selection:select_xz_region()
      :use_designation_marquee(Color4(122, 40, 0, 255))
      :set_cursor('stonehearth:cursors:designate_zone')
      :done(
         function(selector, box)
            local size = {
               x = box.max.x - box.min.x,
               y = box.max.z - box.min.z,
            }
            _radiant.call('stonehearth:designate_trapping_grounds', box.min, size)
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

function TrappingCallHandler:designate_trapping_grounds(session, response, location, size)
   local player_id = session.player_id
   local faction = session.faction
   local size_pt = Point2(size.x, size.y)
   local trapping_grounds = stonehearth.trapping:designate_trapping_grounds(player_id, faction, location, size_pt)
   return { trapping_grounds = trapping_grounds }
end

return TrappingCallHandler
