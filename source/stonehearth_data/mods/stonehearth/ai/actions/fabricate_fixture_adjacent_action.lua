local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local FabricateFixtureAdjacent = class()
FabricateFixtureAdjacent.name = 'fabricate fixture adjacent'
FabricateFixtureAdjacent.does = 'stonehearth:fabricate_fixture_adjacent'
FabricateFixtureAdjacent.args = {
   fabricator = Entity,
   item = Entity,
}
FabricateFixtureAdjacent.version = 2
FabricateFixtureAdjacent.priority = 1

function FabricateFixtureAdjacent:run(ai, entity, args)
   local fixture_fabricator = args.fabricator:get_component('stonehearth:fixture_fabricator')
   if fixture_fabricator then
      ai:execute('stonehearth:run_effect', { effect = 'work' })

      fixture_fabricator:construct(entity, args.item)
   end
end

function FabricateFixtureAdjacent:start(ai, entity)
   ai:set_status_text('building...')
end

return FabricateFixtureAdjacent
