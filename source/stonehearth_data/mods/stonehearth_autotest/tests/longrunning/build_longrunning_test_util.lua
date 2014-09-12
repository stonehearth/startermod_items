local util = {}

function util.get_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

return util
