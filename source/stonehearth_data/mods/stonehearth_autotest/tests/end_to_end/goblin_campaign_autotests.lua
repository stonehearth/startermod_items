local goblin_campaign_autotests = {}
local Point3 = _radiant.csg.Point3

--TODO: write a bunch more tests for all the different ways to get through the scenarios!

--Tests that if the footmen destroys the cage, the wolves escape
--TODO: factor out "jump to challenge"
function goblin_campaign_autotests.wolf_escape_test(autotest)

   local party = stonehearth.unit_control:get_controller('player_1')
                                             :create_party()
   for i=1, 4 do 
      local footman = autotest.env:create_person(-8, 8 + i, { job = 'footman' })
      party:add_member(footman)
   end


   local banner = radiant.entities.create_entity('stonehearth:camp_standard', { owner = 'player_1' })
   radiant.terrain.place_entity(banner, Point3(-40, 1, -40), { force_iconic = false })
   stonehearth.town:get_town('player_1')
                     :set_banner(banner)


   -- Whenever we're at a choice point in the campaign manager, 
   -- select the target that will get us to where we need to go
   local scout_camp_encounter
   local function get_to_the_challenge(...)
         local args = {...}
         local event_name = args[1]
         if event_name == 'elect_node' then
            local _, nodelist_name, nodes = unpack(args)
            if nodelist_name == 'combat' then
               return 'goblin_war', nodes.goblin_war
            end
         elseif event_name == 'override_campaign_arc' then
            local _, campaign, target_arc_type, data = unpack(args)
            if target_arc_type == 'trigger' then
               return 'challenge'
            end
         elseif event_name == 'trigger_arc_edge' then
            local _, arc, edge_name, parent_node = unpack(args)
            if edge_name == 'start' then
               --Jump directly to spawning the wolf cage encounter
               local encounter_uri = arc._sv.encounters._sv.nodelist.create_scout_camp
               local info =  radiant.resources.load_json(encounter_uri)
               scout_camp_encounter = radiant.create_controller('stonehearth:game_master:encounter', info)

               radiant.events.listen('create_scout_camp', 'stonehearth:create_camp_complete', function(e)
                  --When the encounter is finished initializing, tell party to go to cage
                  local wolf_cage = scout_camp_encounter._sv.ctx:get('create_scout_camp.entities.wolf_cage')
                  local location = radiant.entities.get_world_grid_location(wolf_cage)
                  party:place_banner(party.ATTACK, location, 0)

                  --grab ptr to wolf, verify that he escapes the world
                  local wolf_entity = scout_camp_encounter._sv.ctx:get('create_scout_camp.citizens.tame_wolf[1]')
                  autotest.util:succeed_when_destroyed(wolf_entity)
                  return radiant.events.UNLISTEN
               end)

               return 'create_scout_camp', scout_camp_encounter
            end
         end
   end

   --tell GM to start the goblin campaign
   stonehearth.game_master:debug_campaign('combat', function(...)
      local result = { get_to_the_challenge(...) }
      if #result > 0 then
         return unpack(result)
      end
   end)

   --autotest:sleep(1000)
   --assert(scout_camp_encounter)

   autotest:sleep(60000)
   autotest:fail('wolves failed to escape')

end



return goblin_campaign_autotests

--[[
   

   local function get_to_the_challenge(...)
         local args = {...}
         local event_name = args[1]
         if event_name == 'elect_node' then
            local _, nodelist_name, nodes = unpack(args)
            if nodelist_name == 'combatd' then
               return 'goblin_war', nodes.goblin_war
            end
         elseif event_name == 'override_campaign_arc' then
            local _, campaign, target_arc_type, data = unpack(args)
            if target_arc_type == 'trigger' then
               return 'challenge'
            end
         elseif event_name == 'trigger_arc_edge' then
            local _, arc, edge_name, encounter = unpack(args)
            if edge_name == 'start' then
            end
         end
   end

   stonehearth.game_master:debug_campaign('combat', function(...)
         var result = { get_to_the_challenge(...) }
         if #result > 0 then
            return unpack(result)
         end
      end)

]]