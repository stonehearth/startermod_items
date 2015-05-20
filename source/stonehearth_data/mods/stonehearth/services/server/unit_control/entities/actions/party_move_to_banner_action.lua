local Point3 = _radiant.csg.Point3

local PartyMoveToBannerAction = class()

PartyMoveToBannerAction.name = 'party move to banner'
PartyMoveToBannerAction.does = 'stonehearth:party:move_to_banner'
PartyMoveToBannerAction.args = {
   party    = 'table',     -- the party
   location = Point3    -- world location of formation position
}
PartyMoveToBannerAction.version = 2
PartyMoveToBannerAction.priority = 5

function PartyMoveToBannerAction:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._party = args.party
   self._location = args.location + args.party:get_formation_offset(entity)

   -- we are almost certainly too far away.  check once before creating the trace.
   if self:_check_position() then
      return
   end
   
   -- no?  ok, set_think_output whenever we get there   
   self._trace = radiant.entities.trace_grid_location(entity, 'hold formation')
                     :on_changed(function()
                           self:_check_position()
                        end)
end


function PartyMoveToBannerAction:stop_thinking()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

function PartyMoveToBannerAction:_check_position()
   local location = radiant.entities.get_world_grid_location(self._entity)
   if location:distance_to(self._location) <= 2 then
      return false
   end
   
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
   self._ai:set_think_output({
         party = self._party,
         location = self._location,
      })
   return true
end

local ai = stonehearth.ai
return ai:create_compound_action(PartyMoveToBannerAction)
         :execute('stonehearth:goto_location', { reason = 'party move to banner', location = ai.PREV.location  })
         :execute('stonehearth:trigger_event', {
               source = ai.BACK(2).party,
               event_name = 'stonehearth:party:arrived_at_banner',
               event_args = {
                  party = ai.BACK(2).party,
                  member = ai.ENTITY,
               }
            })
