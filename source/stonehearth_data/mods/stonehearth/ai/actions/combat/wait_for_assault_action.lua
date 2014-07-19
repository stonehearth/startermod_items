local log = radiant.log.create_logger('combat')

local WaitForAssault = class()

WaitForAssault.name = 'wait for assault'
WaitForAssault.does = 'stonehearth:combat:wait_for_assault'
WaitForAssault.args = {}
WaitForAssault.think_output = {
   assault_events = 'table'   -- returns an array of assault events (AssaultContexts)
}
WaitForAssault.version = 2
WaitForAssault.priority = 1
WaitForAssault.weight = 1

function WaitForAssault:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity

   local assault_events = stonehearth.combat:get_assault_events(entity)

   if assault_events and next(assault_events) then
      ai:set_think_output({ assault_events = assault_events })
      return
   end

   self:_register_events()
end

function WaitForAssault:stop_thinking(ai, entity, args)
   self:_unregister_events()

   self._ai = nil
   self._entity = nil
end

function WaitForAssault:_register_events()
   if not self._registered then
      radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
      self._registered = true
   end
end

function WaitForAssault:_unregister_events()
   if self._registered then
      radiant.events.unlisten(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
      self._registered = false
   end
end

function WaitForAssault:_on_assault(context)
   -- on rare occassions, this event fails to unsubscribe after returning radiant.events.UNLISTEN
   -- guard against that case here
   if not self._ai then
      return
   end

   if context.attacker:is_valid() then
      local assault_events = { context }
      self._ai:set_think_output({ assault_events = assault_events })

      self._registered = false
      return radiant.events.UNLISTEN
   end
end

return WaitForAssault
