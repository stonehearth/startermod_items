# Overview

Attributes are stats on a character. Attributes can be computed from other attributes. For more details about attributes, see the attributes_component in the reference section. 

# Using attributes: 

To get the attribute_component

      local attributes_component = entity:get_component('stonehearth:attributes')

Make sure the attributes component exists before using it. If no attributes component exists on the object, then it will be nil. 

      if not attributes_component then
         --getting an attribute
         local calorie_value = attributes_component = get_attribute('calories)

         --setting an attribute
         attributes_component:set_attribute('calories', 120)
      end

# Attributes and their function: 

   - **courage** - Measure's a unit's bravery. In humans, is caluclated from spirit. In monsters may be set in json/at creation. 
   - **menace** - Measures scariness of an entity, opposed by another entity's courage. If an entity with a low courage meets an entity with a high menace, the low courage entity may run away/behave differently. This means 2 entities with low courage and high menace may run away from each other. 
   - **sample\_attribute\_name** - (type) function (sample attribute desc. template)