--[[
   The Workshop.lua component implements all the functionality
   associated with the place a crafter goes to do the work a user
   assigns to him. A profession's workplace.txt file (for example,
   carpenter_workbench.txt references this blob of code as a component.

   Conceptually, all workshops have an intermediate item--the item the crafter
   is working on right now. If the crafter is not working on an item,
   the current item is nil. If the crafter is working on an item, then
   the current item has a recipe and a status for its progress.
]]

-- All workshops have a ToDo list through which the user instructs the crafter
local ToDoList = radiant.mods.require('mod://stonehearth_crafter/lib/todo_list.lua')
local Workshop = class()

function Workshop:__init(entity)
   self._todo_list = ToDoList()        -- The list of things we need to work on
   self._entity = entity               -- The entity associated with this component
   self._crafter = {}                  -- The worker component associated with this bench
   self._curr_order = nil              -- The order currently being worked on. Nil until we get an order from the todo list
   self._intermediate_item = nil       -- The item currently being worked on. Nil until we actually start crafting

                                       -- TODO: revise all three of these to use entity-container
   self._bench_ingredients = {}        -- A table of ingredients currently collected (TODO: how to sort by classes of materials?)
   self._bench_outputs = nil           -- An array of finished products on the bench, to be added to the outbox. Nil if nothing.
   self._outbox = {}                   -- An array of finished objects, ready to be used
end

function Workshop:extend(json)
end

function Workshop:get_entity()
   return self._entity
end

function Workshop:get_todo_list()
   return self._todo_list
end

function Workshop:get_intermediate_item()
   return self._intermediate_item
end

function Workshop:get_curr_recipe()
   if(self._curr_order) then
      return self._curr_order:get_recipe()
   else
      return nil
   end
end

--[[
   returns: true if there is an output on the bench, false
   otherwise
]]
function Workshop:has_bench_outputs()
   return self._bench_outputs
end

--[[
   Pops an entity off the bench and returns it
   returns: an entity from the bench, or nil if there
   are no entities left.
]]
function Workshop:pop_bench_output()
   if self._bench_outputs == nil then
      return nil
   end

   local output = table.remove(self._bench_outputs)
   if table.getn(self._bench_outputs) == 0 then
      self._bench_outputs = nil
   end
   return output

end

--[[
   Get the next thing off the top of the todo list.
   Also sets the current order for the workshop.
   Assumes that order is currently nil (because if we're
   halfway through an order, we don't want to nuke it by accdient)
   TODO: later, if we allow a function to let the user cancel the
   current order, make sure it's set to nil before calling next task
   returns: the next recipe on the todo list (if any, nil if there
            is no next order), and a list of ingredients that still
            need to be collected in order to execute the recipe.
]]
function Workshop:get_next_task()
   assert(not self._curr_order, "Current order is not nil; do not get next item")
   local order, remaining_ingredients = self._todo_list:get_next_task()
   self._curr_order = order
   return self:get_curr_recipe(), remaining_ingredients
end

--[[
   We store the items on the bench indexed by their material.
   If there is already an item of that material on the
   bench, just increment its amount # and add its entity to
   the list of associated entities. If not, add a new one.
   TODO: use entity_container
   item: the actual entity of the ingredient
]]
function Workshop:add_item_to_bench(item)
   local material = item:get_component('item'):get_material()
   if(self._bench_ingredients[material]) then
      self._bench_ingredients[material].amount = self._bench_ingredients[material].amount + 1
      table.insert(self._bench_ingredients[material].contents, item)
   else
      self._bench_ingredients[material] = {amount = 1, contents = {item} }
   end
end

--[[
   Given a type of object, determine the # of that
   type on the bench.
   material:   name of the material in question (wood, cloth)
   returns:    num items of type item, or nil if none.
]]
function Workshop:num_items_on_bench(material)
   if not self._bench_ingredients[material] then
      self._bench_ingredients[material] =  {amount = 0, contents = {} }
   end
   return self._bench_ingredients[material].amount
end

--[[
   Associate a crafter component with this bench.
]]
function Workshop:add_crafter(crafter)
   self._crafter = crafter
end

--[[
   Create the item that represents the crafter's progress
   on the current item in the workshop. An intermediate item
   tracks its progress and its entity.
   To create the intermediate item, we must first remove all
   the ingredients from the bench. We assume all the ingredients
   are present. If crafting is interrupted,
   Returns: the intermediate item
]]
function Workshop:create_intemediate_item()
   --verify that the ingredients for the curr recipe are on the bench
   assert(self:verify_curr_recipe(), "Can't find all ingredients in recipe")

   local recipe = self:get_curr_recipe()
   for material, amount in radiant.resources.pairs(recipe.ingredients) do
      --decrement the amount associated with the material
      self._bench_ingredients[material].amount = self._bench_ingredients[material].amount - amount
      local num_removed = 0

      --remove ingredients from bench and from the world
      while num_removed < amount do
         local item_to_remove = table.remove(self._bench_ingredients[material].contents)
         radiant.entities.remove_child(radiant._root_entity, item_to_remove)
         num_removed = num_removed + 1
      end
   end

   --Create intermediate item (with progress) and place its entity in the world
   self._intermediate_item = {progress = 0}
   self._intermediate_item.entity = radiant.entities.create_entity(self._crafter:get_intermediate_item())

   --TODO: use an entity container to put it right over the bench
   radiant.terrain.place_entity(self._intermediate_item.entity, RadiantIPoint3(-12, -5, -12))
   return self._intermediate_item
end

--[[
   Compare the ingredients in the recipe to the ingredients in the bench
   returns: True if all ingredients are on bench. False otherwise.
]]
function Workshop:verify_curr_recipe()
   local recipe = self:get_curr_recipe()
   if recipe then
      for material, amount in radiant.resources.pairs(recipe.ingredients) do
         if not (self:num_items_on_bench(material) >= amount) then
            return false
         end
      end
      --Found all ingredients in for loop
      return true
   else
      return false
   end
end

--[[
   Reset all the things that hold the intermediate state
   and place the workshop outputs into the world.
]]
function Workshop:crafting_complete()
   radiant.entities.remove_child(radiant._root_entity, self._intermediate_item.entity)
   self:produce_outputs()
   self._todo_list:chunk_complete(self._curr_order)
   self._curr_order = nil
   self._intermediate_item = nil
end

--[[
   Produces all the things in the recipe, puts them in the world.
   TODO: handle unwanted outputs, like toxic waste
]]
function Workshop:produce_outputs()
   local recipe = self:get_curr_recipe()
   local outputs = recipe.produces
   self._bench_outputs = {}
   for i, product in radiant.resources.pairs(outputs) do
      local result = radiant.entities.create_entity(product.item)
      --TODO: use entity container to put items on the bench
      radiant.terrain.place_entity(result, RadiantIPoint3(-12, -5, -12))
      table.insert(self._bench_outputs, result)
   end
end


return Workshop