local ReturnTrailingAnimalToPasture = class()

ReturnTrailingAnimalToPasture.name = 'return trailing animal to pasture'
ReturnTrailingAnimalToPasture.does = 'stonehearth:herding'
ReturnTrailingAnimalToPasture.args = {}
ReturnTrailingAnimalToPasture.version = 2
ReturnTrailingAnimalToPasture.priority = stonehearth.constants.priorities.shepherding_animals.RETURN_TO_PASTURE

local ai = stonehearth.ai
return ai:create_compound_action(ReturnTrailingAnimalToPasture)
   :execute('stonehearth:wait_for_trailing_animal')
   :execute('stonehearth:goto_location', {
               reason = 'dropping off some critters!',
               location = ai.PREV.pasture_center
               })
   :execute('stonehearth:run_effect', {effect = 'idle_look_around'})
   --TODO: add an action here to wait for the sheep to get to us?
   :execute('stonehearth:leave_animal_in_pasture', {animal = ai.BACK(3).animal, pasture = ai.BACK(3).pasture_center})
