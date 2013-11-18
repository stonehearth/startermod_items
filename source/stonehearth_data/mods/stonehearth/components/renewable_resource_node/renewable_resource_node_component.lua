local calendar = radiant.mods.load('stonehearth').calendar

local RenewableResourceNodeComponent = class()

function RenewableResourceNodeComponent:__init(entity)
   self._entity = entity
   self._resource = nil       -- the entity spawned when the resource is harvested
   self._renewal_time = nil   -- how long until the resource respawns
   self._calendar_constants = calendar.get_constants();
end
   
function RenewableResourceNodeComponent:extend(json)
   if json.resource then
      self._resource = json.resource
   end
   if json.renewal_time then
      local duration = string.sub(json.renewal_time, 1, -2)
      local time_unit = string.sub(json.renewal_time, -1, -1)

      if time_unit == 'd' then
         duration = duration * self._calendar_constants.hours_per_day
      elseif time_unit == 'm' then
         assert(false, 'durations under 1 hour are not supported for renewable resource nodes!')
      end

      self._renewal_time = tonumber(duration)
   end
end

function RenewableResourceNodeComponent:spawn_resource(location)
   if self._resource then
      local item = radiant.entities.create_entity(self._resource)
      radiant.terrain.place_entity(item, location)

      --start the countdown to respawn.
      local render_info = self._entity:add_component('render_info')
      render_info:set_model_variant('depleted')
      self._renewal_countdown = self._renewal_time
      radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
   end
end

function RenewableResourceNodeComponent:renew(location)
   local render_info = self._entity:add_component('render_info')
   render_info:set_model_variant('')
end

function RenewableResourceNodeComponent:on_hourly()
   if self._renewal_countdown <= 0 then
      radiant.events.unlisten(calendar, 'stonehearth:hourly', self, self.on_hourly)
      self:renew()
   else 
      self._renewal_countdown = self._renewal_countdown - 1
   end
end



return RenewableResourceNodeComponent
