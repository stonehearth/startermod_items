local Entity = _radiant.om.Entity

local WaitForPastureVacancy = class()

WaitForPastureVacancy.name = 'wait for pasture vacancy'
WaitForPastureVacancy.does = 'stonehearth:wait_for_pasture_vacancy'
WaitForPastureVacancy.args = {
   pasture = Entity, --the pasture we're waiting on
   wait_timeout = {  --the amt of time we wait after hearing the pasture is available, to allow other things to run
      type = 'string', 
      default = '0m'
   }
}
WaitForPastureVacancy.version = 2
WaitForPastureVacancy.priority = 1

-- Does the pasture need animals? If so, WAIT so the other dude gets to run. 
-- If not, listen for the pasture's "need animals" event, and then wait, and then run
function WaitForPastureVacancy:start_thinking(ai, entity, args)
   self._args = args
   self._ai = ai
   self._pasture_listener = radiant.events.listen(args.pasture, 'stonehearth:on_pasture_animals_changed', self, self._on_pasture_count_changed)
   self:_on_pasture_count_changed()
end

--Returns true if the pasture needs critters, false otherwise
function WaitForPastureVacancy:pasture_check()
   local pasture_component = self._args.pasture:get_component('stonehearth:shepherd_pasture')
   if pasture_component and 
      pasture_component:get_num_animals() < pasture_component:get_min_animals() then
      return true
   end
   return false
end

-- When we know the pasture count as changed, check if we need to look for more critters
-- If so, and if we have a wait timer, WAIT the specified time, check again, and then set think output.
-- If we don't have a wait timer, then just set think output immediately.
function WaitForPastureVacancy:_on_pasture_count_changed()
   if self:pasture_check() then
      if self._args.wait_timeout == '0m' then
         self._ai:set_think_output()
      else
         self._timer = stonehearth.calendar:set_timer('5m', function()
            self._timer = nil
            if self:pasture_check() then
               --TODO: I've seent this fire from a "finished" state. 
               self._ai:set_think_output()
            end
         end)
      end
   end
end

--If we have a timer or a listener destroy them
function WaitForPastureVacancy:stop_thinking()
   self:_destroy_loose_ends()
end

--Do it here too in case it skips stop_thinking
function WaitForPastureVacancy:stop()
   self:_destroy_loose_ends()
end

function WaitForPastureVacancy:_destroy_loose_ends()
   if self._timer then
      self._timer:destroy()
      self._timer = nil
   end
   if self._pasture_listener then
      self._pasture_listener:destroy()
      self._pasture_listener = nil
   end
end

return WaitForPastureVacancy