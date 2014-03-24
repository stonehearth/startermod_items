--This optional class gates a crop's ability to get to the next level

local corn_class = {}

--Corn can only progress to level 5 corn if it's on really good dirt
function corn_class.growth_check(entity, target_growth_stage, growth_attempts)
   if target_growth_stage > 4 and growth_attempts >= 8 then
      return 8
   end
   if target_growth_stage == 5 then
      --are we on really good dirt?
      local crop_component = entity:get_component('stonehearth:crop')
      local dirt = crop_component:get_dirt_plot()
      local dirt_component = dirt:get_component('stonehearth:dirt_plot')
      local dirt_fertility, dirt_moisture = dirt_component:get_fertility_moisture()
      if dirt_fertility > stonehearth.constants.soil_fertility.GOOD then
         return target_growth_stage
      else
         return target_growth_stage - 1
      end
   end
   return target_growth_stage
end

return corn_class