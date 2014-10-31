local TutorialService = class()
local log = radiant.log.create_logger('tutorial')

function TutorialService:initialize()

end

--[[
function TutorialService:initialize()
   self._sv = self.__saved_variables:get_data()
   self._sv.selected_sub_part = nil

   self._sv.step_sequence = {} -- array of step names (string)
   self._sv.steps = {} -- lookup table for steps
   self._sv.current_step = ''

   self:_add_step('harvest_tree')
   self:_add_step('two')
   self:_add_step('three', self._)
end

function TutorialService:start()
   if not self._sv.initialized then
      -- start all the step watchers
      for k, step in pairs(self._sv.steps) do
         if step.watch then
            step.watch()
         end
      end

      self._sv.initialized = true
      self.__saved_variables:mark_changed()

      radiant.call('stonehearth:spawn_scenario', 'stonehearth:quests:collect_starting_resources')
   end
end

function TutorialService:complete_step(name)
   local step = self._sv.steps[name]

   if step ~= nil then
      if step.unwatch then
         step.unwatch()
      end
      step.finished = true
   end

   self._sv.current_step = self:_get_next_step()

end

function TutorialService:_add_step(name, watch_function, unwatch_function)
   local step = {
      name = name,
      watch = watch_function,
      unwatch = unwatch_function,
   }

   self._sv.steps[name] = step;
   table.insert(self._sv.step_sequence, step.name)

   self.__saved_variables:mark_changed()
end

function TutorialService:_get_next_step(session, response)

   for i, name in ipairs(self._sv.step_sequence) do
      local step = self._sv.steps[name]
      if step ~= nil and not step.finished then
         return name
      end
   end

   response:reject()
end

]]
return TutorialService
