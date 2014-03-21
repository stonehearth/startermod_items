local GrowingComponent = class()

function GrowingComponent:initialize(entity, json)
   self._entity = entity
   self._growth_period = json.growth_period or '1h'
   self._growth_stages = json.growth_stages
   self._growth_stages = json.growth_stages

   self._sv = self.__saved_variables:get_data()
   if not self._sv.current_growth_stage then
      self._sv.current_growth_stage = 1
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
   --TODO: use events to tell crop component to suck nutrients out of the soil?
   --TODO: stop growth if some condition is not met
   self._sv.current_growth_stage = math.min(self._sv.current_growth_stage + 1, #self._growth_stages)
   self.__saved_variables:mark_changed()

   local finished = self._sv.current_growth_stage >= #self._growth_stages
   local stage = self._growth_stages[self._sv.current_growth_stage]

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

function GrowingComponent:_apply_current_stage()
   local stage_data = self._growth_stages[self._sv.current_growth_stage]

   self._entity:add_component('render_info')
                     :set_model_variant(stage_data.model_name)

   radiant.entities.set_name(self._entity, stage_data.name)
   radiant.entities.set_description(self._entity, stage_data.description)
end

--- When we're done growing, stop the growth timer
function GrowingComponent:_stop_growing()
   if self._growth_timer then
      self._growth_timer:destroy()
      self._growth_timer = nil
   end
end

return GrowingComponent
