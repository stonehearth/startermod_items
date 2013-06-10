--[[
   After an item has been crafted, add its outputs to the outbox.
   TODO: Add the small version of each item
   TODO: Read the appropriate outbox location from the crafter's class_info.txt
   TODO: Make the outbox an actual entity container?
   TODO: If there are side-effect products, like toxic waste, handle separately
]]
local FillOutboxAction = class()

FillOutboxAction.name = 'stonehearth.actions.fill_outbox'
FillOutboxAction.does = 'stonehearth_crafter.activities.fill_outbox'
FillOutboxAction.priority = 5

function FillOutboxAction:__init(ai, entity)
   self._outputs = {}
   
   --TODO: Use actual outbox when we get more actions
   self._output_x = -9
   self._output_y = -9
end

--[[
   Run over to the outbox and put all the outputs in it.
   Create all the outputs and save them first so that we
   can just dump them all in the outbox sans animation
   if we're forced to stop. 
   recipe: the recipe we just crafted
]]
function FillOutboxAction:run(ai, entity, recipe)
   local outputs = recipe.produces

   for i, product in radiant.resources.pairs(outputs) do
      local result = radiant.entities.create_entity(product.item)
      result.placed = false
      table.insert(self._outputs, result)
   end

   for v, item in ipairs(self._outputs) do
      local goto = RadiantIPoint3(self._output_x, 1, self._output_y)
      --TODO: add carry animation
      ai:execute('stonehearth.activities.goto_location', goto)
      --TODO: add some puttong-down animation
      radiant.terrain.place_entity(item, RadiantIPoint3(self._output_x + 1, 1, self._output_y))
      self._output_y = self._output_y + 1
      item.placed = true
   end
   self._outputs = {}
end

--[[
   If interrupted, unplaced items just appear in the outbox
   withut animation.
]]
function FillOutboxAction:stop()
   for v, item in ipairs(self._outputs) do
      if(not item.placed) then       
         radiant.terrain.place_entity(item, RadiantIPoint3(self._output_x + 1, 1, self._output_y))
      end
   end
   self._outputs = {}
end

return FillOutboxAction