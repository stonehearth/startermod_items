local AddBuff = class()
AddBuff.name = 'add buff'
AddBuff.does = 'stonehearth:add_buff'
AddBuff.args = {
   buff = 'string'      -- the buff to add
}
AddBuff.version = 2
AddBuff.priority = 1

function AddBuff:start(ai, entity, args)
   -- this is somewhat broken.  buffs should be refcounted, right?  stop is going to
   -- remove it uncondtionally...
   if not radiant.entities.has_buff(entity, args.buff) then
      radiant.entities.add_buff(entity, args.buff);
   end
end

function AddBuff:stop(ai, entity, args)
   radiant.entities.remove_buff(entity, args.buff);
end

return AddBuff
