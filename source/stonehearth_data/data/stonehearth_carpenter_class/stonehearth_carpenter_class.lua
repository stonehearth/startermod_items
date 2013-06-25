--[[
   The carpenter api implements all the functionality that other files will need.
]]

local carpenter_class = {}

function carpenter_class.promote(entity)
    radiant.entities.inject_into_entity(entity, '/stonehearth_carpenter_class/class_info/')
end

return carpenter_class