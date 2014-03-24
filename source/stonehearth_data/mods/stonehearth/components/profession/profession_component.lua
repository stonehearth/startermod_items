--[[
   This component stores data about the job that a settler has.
]]

local ProfessionComponent = class()

function ProfessionComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if self._sv.profession_uri then
      local json = radiant.resources.load_json(self._sv.profession_uri, true)
      if json then
         self:_load_profession_script(json)
         self:_call_profession_script('restore', json)
      end
   end
end

function ProfessionComponent:get_profession_id()
   return self._sv.profession_id
end

function ProfessionComponent:promote_to(profession_uri)
   local json = radiant.resources.load_json(profession_uri, true)
   if json then
      self:demote()
      self._sv.profession_uri = profession_uri
      self._sv.profession_id = json.profession_id
      self:_load_profession_script(json)
      self:_set_unit_info(json)
      self:_equip_outfit(json)
      self:_call_profession_script('promote', json)

      --Let people know that the promotion has (probably) happened.
      -- xxx: is there a better way?  How about if the town listens to all 'stonehearth:profession_changed'
      -- messages from its citizens?  That sounds good!!
      radiant.events.trigger(stonehearth.object_tracker, 'stonehearth:promote', { entity = self._entity })

      -- so good!  keep this one, lose the top one.  too much "collusion" between components =)
      radiant.events.trigger(self._entity, 'stonehearth:profession_changed', { entity = self._entity })
      self.__saved_variables:mark_changed()
   end
end

function ProfessionComponent:demote()
   self:_remove_outfit()
   self:_call_profession_script('demote')
   self._sv.profession_id = nil
   self.__saved_variables:mark_changed()
end

function ProfessionComponent:_set_unit_info(json)
   if json and json.name then
      self._entity:add_component('unit_info'):set_description(json.name)
   end
end

function ProfessionComponent:_equip_outfit(json)
   if json and json.outfit then
      self._sv.outfit = radiant.entities.create_entity(json.outfit)
      local equipment = self._entity:add_component('stonehearth:equipment')
      equipment:equip_item(self._sv.outfit)
   end
end

function ProfessionComponent:_remove_outfit()
   if self._sv.outfit then
      self._entity:add_component('stonehearth:equipment'):unequip_item(self._sv.outfit)
      radiant.entities.destroy_entity(self._sv.outfit)
      self._sv.outfit = nil
   end
end

function ProfessionComponent:_load_profession_script(json)
   self._profession_script = nil
   if json.script then
      self._profession_script = radiant.mods.load_script(json.script)
   end
end

function ProfessionComponent:_call_profession_script(fn, ...)
   local script = self._profession_script
   if script and script[fn] then
      script[fn](self._entity, ...)
   end
end

return ProfessionComponent
