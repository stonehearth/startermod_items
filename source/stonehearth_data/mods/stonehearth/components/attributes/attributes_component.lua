--[[
   Attributes are numbers associated with an entity. There are a few kinds of attributes
   - basic attributes are just integers 
   - random_range attributes will get you a number between base and max, inclusive
   - derived attributes derived from equations relating other attributes. 

   Here are the 3 example types as described in json:
   "ferocity": {
      "type" : "basic",
      "value" : 1
   },
   "compassion_modifier": {
      "type" : "random_range",
      "base" :  1,
      "max"  :  9
   },
   "health": {
      "type" : "derived", 
      "equation" : "muscle + willpower + discipline"
   }, 

   Attributes can have alphabetical characters in them, + the _ character. (But they cannot have
   numeric characters in them)

   It's currently the attribute author's job to make sure that derived attributes
   do not have loops in their calculation.

   Attributes can be altered via modifiers. Thus, an attribute's base value is an integer, if
   it's a basic attribute, or the number derived from its equation, if it's a derived attribute. 
   All attributes also have an effective value, which is the base value + it's modifiers. 

   TODO: - Optimizations: check for loops
         - make declaring attributes easier
         - design: do the derived attributes make the player feel that the differences are bigger than 
                   are actually important? This is the opposite of what we want
]]

local AttributeModifier = require 'components.attributes.attribute_modifier'
local rng = _radiant.csg.get_default_rng()

local AttributesComponent = class()

function AttributesComponent:__init()
   self._attributes = {}
   self._modifiers = {}
   self._dependencies = {}
end

--- Given the type of attribute, handle as needed
function AttributesComponent:__create(entity, json)
   self._entity = entity
   self.__savestate = radiant.create_datastore({
         attributes = self._attributes,
         modifiers = self._modifiers,
         dependencies = self._dependencies,
      })

   if json then
      self._incoming_json = json
      for n, v in pairs(json) do
         self:_add_new_attribute(n, v)
      end
      self._incoming_json = nil
      self.__savestate:mark_changed()
   end
end

function AttributesComponent:restore(entity, saved_variables)
   self._entity = entity
   self.__savestate = saved_variables
   saved_variables:read_data(function(o)
         self._attributes = o.attributes
         self._modifiers = o.modifiers
         self._dependencies = o.dependencies
      end)
end

--- Given a name of an attribute and data, add it to self._attributes
--  Do not replace existing entries, if already calculated.
function AttributesComponent:_add_new_attribute(name, data)
   if not self._attributes[name] then 
      local new_attribute = {}
      new_attribute.type = data.type
      if new_attribute.type == 'basic' then
         new_attribute.value = data.value
      elseif new_attribute.type == 'random_range' then
         new_attribute.value = rng:get_int(data.base, data.max)
      elseif new_attribute.type == 'derived' then
         new_attribute.equation = data.equation
         self:_load_dependencies(data.equation, name)
         new_attribute.value = self:_evaluate_equation(data.equation)
      end
      new_attribute.effective_value = new_attribute.value
      self._attributes[name] = new_attribute
   end
end

--- Find all the variables in the string and register this attribute
--  as dependent upon them. 
function AttributesComponent:_load_dependencies(equation, attribute)
   for variable_name in string.gmatch(equation, "[%a_]+") do
      local dependency_table = self._dependencies[variable_name]
      if not dependency_table then
         dependency_table = {}
         self._dependencies[variable_name] = dependency_table
      end
      table.insert(dependency_table, attribute)
   end
end

--- Given a string equation with variables, sub all variables for
--  their effective value
function AttributesComponent:_substitute_for_variables(equation)
   return string.gsub(equation, "([%a_]+)", function(variable_name)
      --Find value of variable name. If nothing, return zero
      local value = self:get_attribute(variable_name)
      if value then
         return value
      else
         return 0
      end
   end)
end

--- Given a string equation with variable, sub in values and evaluate
--  @param - an equation string (eg: 'foo * bar + baz')
--  @returns - the calculated value of the equation
function AttributesComponent:_evaluate_equation(equation)
   local revised_equation = self:_substitute_for_variables(equation)
   local fn = loadstring('return ' .. revised_equation)
   return fn()
end

--- Get the value of an attribute. 
--  If it doesn't exist, create it. If there is json already for it, incoming but
--  not yet processed, process the children first.
function AttributesComponent:get_attribute(name)
   if not self._attributes[name] then
      --check if this attribute just hasn't been processed yet
      --If not, then process it (TODO: hope there's no circular dependencies)
      if self._incoming_json and self._incoming_json[name] then
         self:_add_new_attribute(name,  self._incoming_json[name])
      else
         self._attributes[name] = {
            type = 'basic',
            value = 0
         }
      end
   end

   if self._attributes[name].effective_value then
      return self._attributes[name].effective_value
   else
      return self._attributes[name].value
   end
end

--Note: now, only use this on basic/random values. Everything
--else recalculates automatically 
function AttributesComponent:set_attribute(name, value)
   if not self._attributes[name] then
      self._attributes[name] = {
         type = 'basic',
         value = 0
      }
   else
      --If it already exists, just make sure it's not derived
      assert(self._attributes[name].type ~='derived')
   end

   if value ~= self._attributes[name].value then
      self._attributes[name].value = value
      self._attributes[name].effective_value = nil
      self:_recalculate(name)
   end
end

function AttributesComponent:modify_attribute(attribute)
   local modifier = AttributeModifier(self, attribute)
   
   if not self._modifiers[attribute] then
      self._modifiers[attribute] = {}
   end
   
   table.insert(self._modifiers[attribute], modifier)
   return modifier
end

function AttributesComponent:_remove_modifier(attribute, attribute_modifier)
   for i, modifier in ipairs(self._modifiers[attribute]) do
      if modifier == attribute_modifier then
         table.remove(self._modifiers[attribute], i)
         self:_recalculate(attribute)
         break
      end
   end
end

function AttributesComponent:_recalculate(name)
   local mult_factor = 1.0
   local add_factor = 0.0
   local min = nil
   local max = nil

   for attribute_name, modifier_list in pairs(self._modifiers) do
      if attribute_name == name then
         for _, modifier in ipairs(modifier_list) do
            local mods = modifier:_get_modifiers();

            if mods['multiply'] then
               mult_factor = mult_factor * mods['multiply']
            end

            if mods['add'] then
               add_factor = add_factor + mods['add']
            end

            if mods['min'] then
               if min then
                  min = math.min(min, mods['min'])
               else 
                  min = mods['min']
               end
            end

            if mods['max'] then
               if max then
                  max = math.min(max, mods['max'])
               else
                  max = mods['max']
               end
            end
         end   
      end
   end

   -- careful, order is important here!
   -- Note: we assume that if this element is a derived element, it's value is already calculated.
   self._attributes[name].effective_value = self._attributes[name].value * mult_factor
   self._attributes[name].effective_value = self._attributes[name].effective_value + add_factor
   if min then
      self._attributes[name].effective_value = math.max(min, self._attributes[name].effective_value)
   end
   if max then
      self._attributes[name].effective_value = math.min(max, self._attributes[name].effective_value)
   end

   radiant.events.trigger(self._entity, 'stonehearth:attribute_changed:' .. name, { 
         name = name,
         value = self._attributes[name].effective_value
      })

   --Recalculate all the attributes that depend on this one
   --TODO: hope there's no loops! 
   if self._dependencies[name] then
      for i, v in pairs(self._dependencies[name]) do
         self._attributes[v].value = self:_evaluate_equation(self._attributes[v].equation)
         self:_recalculate(v)
      end
   end
   self.__savestate:mark_changed()
end

return AttributesComponent
