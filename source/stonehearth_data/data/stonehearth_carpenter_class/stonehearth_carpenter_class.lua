--[[
   The carpenter api implements all the functionality that other files will need. 
]]

local api = {}

function api.promote(entity)   
    radiant.entities.inject_into_entity(entity, 'mod://stonehearth_carpenter_class/class_info/')
end

return api