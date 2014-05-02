local Entity = _radiant.om.Entity
local ReserveEntity = class()

ReserveEntity.name = 'reserve entity'
ReserveEntity.does = 'stonehearth:reserve_entity'
ReserveEntity.args = {
   entity = Entity,           -- entity to reserve
}
ReserveEntity.version = 2
ReserveEntity.priority = 1

function ReserveEntity:start_thinking(ai, entity, args)
   if not stonehearth.ai:can_acquire_ai_lease(args.entity, entity) then     
      -- the code used to just return here.  after all, if we're going to abort
      -- on start, can't we just not call set_think_output() and let some other
      -- branch of whatever activity is requesting the reservation take over?
      -- well, sometimes this is the *only* branch available, and by never calling
      -- set_think_output() we hang this entire sub-activity.  in many cases, if
      -- only we let everyone above us in the action tree think again, they could
      -- find something much more suitable (e.g. there are many MANY logs in the world
      -- that a worker could pickup, but the pathfinder looking for them happened to
      -- pick one *JUST* before a reservation got slapped on it.  if we ask it to look
      -- again, it will simply skip over that one and likely find one that's not reserved)
      --
      -- so what are we to do?  if we call set_thinking_output() immediately, start()
      -- is almost certainly going to abort().  if we never call set_think_output(), we
      -- have to wait for some other influence to kick our parent and start it thinking
      -- again.  ideally the ai system would have a call which meant "yo bro, i don't
      -- know how we got here but this just isn't going to work.  you need to back way
      -- up and start over".  Then if every unit in a frame had entered that state, the
      -- frame would simply start_thinking() over.  Maybe one day I will implement this,
      -- but we JUST stabilized the AI system and I don't want to wreak havoc on it yet
      -- again. -- tony
      ai:get_log():debug('%s cannot acquire ai lease on %s.  future abort is quite likey.', args.entity, entity)
      self._start_thinking_timer = radiant.set_realtime_timer(100, function ()
            self._start_thinking_timer = nil
            ai:set_think_output()
         end)
      return
   end
   ai:set_think_output()
end

function ReserveEntity:stop_thinking(ai, entity, args)
   -- if the start thinking timer is still valid when stop_thinking is called, some
   -- other sibling or cousin action won the election and is getting ready to run.
   -- kill the timer since it's no longer needed (and we have absoultely *no* desire
   -- to run if the timer exists!)
   if self._start_thinking_timer then
      self._start_thinking_timer:destroy()
      self._start_thinking_timer = nil
   end
end

function ReserveEntity:start(ai, entity, args)
   local target = args.entity

   if not stonehearth.ai:acquire_ai_lease(target, entity) then
      ai:abort('could not lease %s (%s has it).', tostring(target), tostring(stonehearth.ai:get_ai_lease_owner(target)))
      return
   end
   self._acquired_lease = true
end

function ReserveEntity:stop(ai, entity, args)   
   if self._acquired_lease then
      stonehearth.ai:release_ai_lease(args.entity, entity)
      self._acquired_lease = false
   end
   self._lease_component = nil
end

return ReserveEntity
