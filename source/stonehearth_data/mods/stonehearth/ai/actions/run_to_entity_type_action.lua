local Entity = _radiant.om.Entity
local RunToEntityType = class()

RunToEntityType.name = 'run to entity type'
RunToEntityType.does = 'stonehearth:goto_entity_type'
RunToEntityType.args = {
   filter_fn = 'function',
   description = 'string',             -- description of the initiating compound task (for debugging)
   range = {
      default = 128,
      type = 'number',
   },
}
RunToEntityType.think_output = {
   destination_entity = Entity
}
RunToEntityType.version = 2
RunToEntityType.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(RunToEntityType)
         :execute('stonehearth:find_path_to_entity_type', {
            filter_fn = ai.ARGS.filter_fn,
            description = ai.ARGS.description,
            range = ai.ARGS.range
         })
         :execute('stonehearth:follow_path', {
            path = ai.PREV.path,
         })
         :set_think_output({ destination_entity = ai.PREV.path:get_destination()})
