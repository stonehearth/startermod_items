local Entity = _radiant.om.Entity

local AddBuff = class()
AddBuff.name = 'add buff'
AddBuff.does = 'stonehearth:add_buff'
AddBuff.args = {
   buff = 'string',      -- the buff to add
   target = Entity      -- the entity to add the buff to
}
AddBuff.version = 2
AddBuff.priority = 1

function AddBuff:start(ai, entity, args)
   -- this is somewhat broken.  buffs should be refcounted, right?  stop is going to
   -- remove it uncondtionally...
   if not radiant.entities.has_buff(args.target, args.buff) then
      radiant.entities.add_buff(args.target, args.buff);
   end
end

function AddBuff:stop(ai, entity, args)
   radiant.entities.remove_buff(args.target, args.buff);
end

return AddBuff
