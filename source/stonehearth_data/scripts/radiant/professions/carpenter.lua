local Carpenter = class()

local md = require 'radiant.core.md'
md:register_msg_handler("radiant.professions.carpenter", Carpenter)

Carpenter['radiant.md.create'] = function(self, entity)
   self.entity = entity
   om:add_rig_to_entity(entity, 'models/carpenter_outfit.qb')
end

Carpenter['radiant.md.destroy'] = function(self)
   om:remove_rig_from_entity(self.entity, 'models/carpenter_outfit.qb')
end
