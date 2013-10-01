local combat = {}

function combat.get_target_table_top(entity, table_name)
   local target_tables = entity:get_component('target_tables')
   local top_entry = target_tables:get_top(table_name)

   if top_entry then
      return top_entry.target
   end

   return nil
end

function combat.compare_attribute(entity_a, entity_b, attribute)
   local attributes_a = entity_a:get_component('attributes')
   local attributes_b = entity_b:get_component('attributes')

   if attributes_a and attributes_b then
      local ferocity_a = attributes_a:get_attribute(attribute)
      local ferocity_b = attributes_b:get_attribute(attribute)

      return ferocity_a - ferocity_b
   end

   return 0
end

return combat
