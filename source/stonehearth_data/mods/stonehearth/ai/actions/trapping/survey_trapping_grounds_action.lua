local Entity = _radiant.om.Entity

local SurveyTrappingGrounds = class()

SurveyTrappingGrounds.name = 'survey trapping grounds'
SurveyTrappingGrounds.status_text = 'survey trapping grounds'
SurveyTrappingGrounds.does = 'stonehearth:trapping:survey_trapping_grounds'
SurveyTrappingGrounds.args = {
   trapping_grounds = Entity
}
SurveyTrappingGrounds.version = 2
SurveyTrappingGrounds.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(SurveyTrappingGrounds)
   :execute('stonehearth:goto_entity', {
      entity = ai.ARGS.trapping_grounds
   })
   :execute('stonehearth:create_proxy_entity', {
      reason = 'surveying trapping grounds',
      location = ai.PREV.point_of_interest,
      place_on_standable_point = true,
   })
   :execute('stonehearth:turn_to_face_entity', {
      entity = ai.PREV.entity
   })
   :execute('stonehearth:run_effect', {
      effect = 'idle_look_around'
   })
