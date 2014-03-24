local GrowingComponent = class()

function GrowingComponent:initialize(entity, json)
   self._entity = entity
   self._growth_period = json.growth_period or '1h'
   self._growth_stages = json.growth_stages
   self._growth_check_script_uri = json.growth_check_script
   if self._growth_check_script_uri then
      self._growth_check_script = radiant.mods.load_script(self._growth_check_script_uri)
   end

   self._sv = self.__saved_variables:get_data()
   if not self._sv.current_growth_stage then
      self._sv.current_growth_stage = 1
      self._sv.growth_attempts = 0
   end

   self:_apply_current_stage()
   self._growth_timer = stonehearth.calendar:set_interval(self._growth_period, function()
         self:_grow()
      end)
end

function GrowingComponent:destroy()
   self:_stop_growing()
end

--- Do whatever is necessary to get to the next step of growth
function GrowingComponent:_grow()
   self._sv.growth_attempts = self._sv.growth_attempts + 1
   local next_stage = math.min(self._sv.current_growth_stage + 1, #self._growth_stages)

   --TODO: if we have a checker, load it and jump us to whatever growth state it thinks
   --we should be at. Otherwise, just use the next stage (or the max # of stages)
   if self._growth_check_script and self._growth_check_script.growth_check then
       next_stage = self._growth_check_script.growth_check(self._entity, next_stage, self._sv.growth_attempts)
   end

   --If the next stage != the current one, update the model and data to the new stage
   if next_stage ~= self._sv.current_growth_stage then
      self._sv.current_growth_stage = next_stage

      local finished = self._sv.current_growth_stage >= #self._growth_stages
      local stage = self._growth_stages[self._sv.current_growth_stage]

      --TODO: use events to tell crop component to suck nutrients out of the soil?
      radiant.events.trigger(self._entity, 'stonehearth:growing', {
            entity = self._entity,
            stage = stage and stage.model_name or nil,
            finished = finished,
         })

      --TODO: run growth effect, if we have one
      self:_apply_current_stage()   
      if finished then
         self:_stop_growing()
      end
   end

   --Tell the save service to save this data. 
   self.__saved_variables:mark_changed()
end

function GrowingComponent:_apply_current_stage()
   local stage_data = self._growth_stages[self._sv.current_growth_stage]

   --It's possible that the stage has the same model as the last. If 
   --there is no model specified for the stage, keep it the same
   if stage_data.model_name then
      self._entity:add_component('render_info')
                     :set_model_variant(stage_data.model_name)
   end

   if stage_data.name and stage_data.description then
      radiant.entities.set_name(self._entity, stage_data.name)
      radiant.entities.set_description(self._entity, stage_data.description)
   end
end

--- When we're done growing, stop the growth timer
function GrowingComponent:_stop_growing()
   if self._growth_timer then
      self._growth_timer:destroy()
      self._growth_timer = nil
   end
end

return GrowingComponent
