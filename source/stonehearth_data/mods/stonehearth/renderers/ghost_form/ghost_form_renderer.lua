local Point3 = _radiant.csg.Point3

local GhostFormRenderer = class()

function GhostFormRenderer:initialize(render_entity, ghost_form)
   render_entity:set_material_override('materials/ghost_item.xml')
end

return GhostFormRenderer