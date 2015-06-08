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
      -- *** UPDATE ***
      -- we now call :halt() to implement the 'Maybe one day...' case mentioned
      -- below.  This comment preserved for posterity... - tony
      --
      -- [blockquote]
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
      -- [/blockquote]
      --
      local reason = string.format('%s cannot acquire ai lease on %s.', tostring(entity), tostring(args.entity))
      ai:get_log():debug('halting: ' .. reason)
      ai:halt(reason)
      return
   end
   ai:get_log():debug('no one holds lease yet.')
   ai:set_think_output()
end

function ReserveEntity:start(ai, entity, args)
   local target = args.entity

   ai:get_log():debug('trying to acquire lease...')
   if not stonehearth.ai:acquire_ai_lease(target, entity) then
      ai:abort('could not lease %s (%s has it).', tostring(target), tostring(stonehearth.ai:get_ai_lease_owner(target)))
      return
   end
   ai:get_log():debug('got lease!')
   self._acquired_lease = true
end

function ReserveEntity:stop(ai, entity, args)   
   if self._acquired_lease then
      stonehearth.ai:release_ai_lease(args.entity, entity)
      self._acquired_lease = false
   end
end

return ReserveEntity
