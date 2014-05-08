App.StonehearthCraftersView = App.View.extend({
   templateName: 'crafters',

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:profession" : {
               "profession_uri" : {}
            },
            "unit_info": {},
            "stonehearth:crafter" : {
               "workshop" : {}
            }
         }
      }
   },

   init: function() {
      var self = this;
      this._super();

      var pop = App.population.getData();
      this.set('context', pop);
      this._buildCitizensArray();
   },

   actions: {
      craft: function(crafter) {
         var workshop = crafter['stonehearth:crafter']['workshop']['workshop_entity'];
         $(top).trigger("radiant_show_workshop_from_crafter", {
            event_data: {
               workshop: workshop
            }
         });
      },
      placeWorkshop: function(crafter) {
      }
   },

   preShow: function() {
      this._buildCitizensArray();
   },

   _buildCitizensArray: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
     
      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k != "__self" && citizenMap.hasOwnProperty(k)) {
               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
    },

    _onCitizenCrafterComponentChanged: function() {
      var arr = this.get('context.citizensArray');

      this.set('context.citizensArray', arr);
    }.observes('context.citizens.[]'),

    /*
    _foo: function() {
      $.each(this.get('context.citizensArray'), function(i, citizen) {
         var isCrafter = false;

         if (citizen.get('stonehearth:crafter')) {
            isCrafter = true;
         }

         citizens.set('isCrafter', isCrafter);
      });
    }.observes('context.citizensArray.@each.stonehearth:profession.profession_uri.alias'),
    */

});
