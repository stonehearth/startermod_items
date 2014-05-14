local TargetTable = require 'components.target_tables.target_table'

local TargetTables = class()

function TargetTables:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()

   if true then
   --if not self._sv.initialized then
      self._sv.target_tables = {}
      self._sv.initialized = true
   else
      for name, table_data in pairs(self._sv.target_tables) do
         -- safe to reassign keys in pairs loop
         local table_instance = self:_create_target_table(table_data)
         self._sv.target_tables[name] = table_instance
      end
   end

   -- ten second or minute poll is sufficient
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._clean_target_tables)
end

function TargetTables:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:very_slow_poll', self, self._clean_target_tables)
end

function TargetTables:get_target_table(table_name)
   local target_table = self._sv.target_tables[table_name]

   if target_table == nil then
      target_table = self:_create_target_table()
      self._sv.target_tables[table_name] = target_table
   end

   return target_table
end

function TargetTables:_create_target_table(datastore)
   datastore = datastore or radiant.create_datastore()

   local target_table = TargetTable()
   target_table.__saved_variables = datastore
   target_table:initialize()
   return target_table
end

function TargetTables:_clean_target_tables()
   for _, target_table in pairs(self._sv.target_tables) do
      target_table:remove_invalid_targets()
   end
end

return TargetTables
