local TargetTable = require 'components.target_tables.target_table'

local TargetTables = class()

function TargetTables:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.target_tables = {}
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   end
end

function TargetTables:destroy()
   for _, target_table in pairs(self._sv.target_tables) do
      target_table:destroy()
   end
end

function TargetTables:get_target_table(table_name)
   local target_table = self._sv.target_tables[table_name]

   if target_table == nil then
      target_table = radiant.create_controller('stonehearth:target_table')
      self._sv.target_tables[table_name] = target_table
   end

   return target_table
end

return TargetTables
