local rng = _radiant.csg.get_default_rng()

local RandomOutEdgeEncounter = class()

--TODO: on Tuesday, START HERE, WITH 30% spawn chance for undead

function RandomOutEdgeEncounter:start(ctx, info)
   local selection_table = {}
   for out_edge_name, info in pairs(info.out_edges) do
      for i=1, info.weight do 
         table.insert(selection_table, out_edge_name)
      end
   end
   local raffle = rng:get_int(1, #selection_table)
   local out_edge = selection_table[raffle]
   if out_edge and out_edge ~= 'none' then
      assert(type(out_edge) == 'string')
      self._sv.resolved_out_edge = out_edge
   end
   ctx.arc:trigger_next_encounter(ctx)
end

function RandomOutEdgeEncounter:get_out_edge()
   return self._sv.resolved_out_edge
end

function RandomOutEdgeEncounter:stop()
end

return RandomOutEdgeEncounter