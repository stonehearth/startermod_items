local job_helper = {}

--Make initializtion common to all classes
function job_helper.initialize(sv, entity)
   sv._entity = entity
   sv.last_gained_lv = 0
   sv.is_current_class = false
   sv.is_max_level = false
   sv.attained_perks = {}
   sv.parent_level_requirement = 0
end

-- Save promotion-related variables common to all classes
function job_helper.promote(sv, json)
   sv.is_current_class = true
   sv.job_name = json.name
   sv.max_level = json.max_level

   if not sv.is_max_level then
      sv.is_max_level = false
   end

   if json.parent_level_requirement then
      sv.parent_level_requirement = json.parent_level_requirement
   end

   if json.xp_rewards then
      sv.xp_rewards = json.xp_rewards
   end

   if json.level_data then
      sv.level_data = json.level_data
   end

   --By default, all classes participate in worker defense. 
   --Some classes, like the shepherd opt out.
   sv.worker_defense_participant = true
   if json.worker_defense_opt_out then
      sv.worker_defense_participant = false
   end
end

-- Do level up logic common to all classes
function job_helper.level_up(sv)
   sv.last_gained_lv = sv.last_gained_lv + 1

   if sv.last_gained_lv == sv.max_level then
      sv.is_max_level = true
   end
end

-- Add equipment to the character, and also to a tracking table for removal purposes later
function job_helper.add_equipment(sv, args)
   sv[args.equipment] = radiant.entities.create_entity(args.equipment)
   radiant.entities.equip_item(sv._entity, sv[args.equipment])
end

-- Remove equipment from the character
function job_helper.remove_equipment(sv, args)
   if sv[args.equipment] then
      radiant.entities.unequip_item(sv._entity, sv[args.equipment])
      sv[args.equipment] = nil
   end
end

return job_helper