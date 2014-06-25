local MicroWorld = require 'lib.micro_world'
local EmbarkTest = class(MicroWorld)
local personality_service = stonehearth.personality
local Point3 = _radiant.csg.Point3

--Create a tiny embark scenario to make sure initial gameplay functions correctly

function EmbarkTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local bush = self:place_item('stonehearth:berry_bush', 4, 4)
   local bush = self:place_item('stonehearth:berry_bush', -1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -4, 4)
   local bush = self:place_item('stonehearth:berry_bush', 4, -4)

   local bush = self:place_item('stonehearth:berry_bush', 2, 4)
   local bush = self:place_item('stonehearth:berry_bush', 1, -1)
   local bush = self:place_item('stonehearth:berry_bush', -2, 4)
   local bush = self:place_item('stonehearth:berry_bush', 5, -4)

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, -10)
   local tree = self:place_tree(-12, -8)
   local tree = self:place_tree(-12, -6)
   local tree = self:place_tree(-12, -4)
   local tree = self:place_tree(-12, -2)
   local tree = self:place_tree(-12, 0)
   local tree = self:place_tree(-12, 2)

   local critter1 = self:place_item('stonehearth:red_fox', 2, 2)


   local worker = self:place_citizen(-5, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker, description = 'person_embarks'})

   local worker2 = self:place_citizen(-6, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker2, description = 'person_embarks'})

   local worker3 = self:place_citizen(7, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker3, description = 'person_embarks'})

   local worker4 = self:place_citizen(8, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker4, description = 'person_embarks'})

   local worker5 = self:place_citizen(-9, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker5, description = 'person_embarks'})

   local worker6 = self:place_citizen(-10, -5)
   radiant.events.trigger_async(personality_service, 'stonehearth:journal_event', 
                          {entity = worker6, description = 'person_embarks'})
   
   local player_id = worker:get_component('unit_info'):get_player_id()

   self:place_item('stonehearth:firepit', 0, 11, player_id)

   local pop = stonehearth.population:get_population(player_id)
   radiant.entities.pickup_item(worker, pop:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker2, pop:create_entity('stonehearth:oak_log'))
   radiant.entities.pickup_item(worker3, pop:create_entity('stonehearth:trapper:knife'))
   radiant.entities.pickup_item(worker4, pop:create_entity('stonehearth:carpenter:saw'))

   --Place a banner
   local town = stonehearth.town:get_town(player_id)
   local location = Point3(7, 0, 7)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location)
   town:set_banner(banner_entity)


   local session = {
      player_id = player_id,
      kingdom = radiant.entities.get_kingdom(worker)
   }

   stonehearth.farming:add_crop_type(session, 'stonehearth:cotton_crop')


   -- Introduce a new person
   --self:at(10000,  function()
   --      stonehearth.dynamic_scenario:force_spawn_scenario('Immigration')
   --   end)

end

return EmbarkTest

