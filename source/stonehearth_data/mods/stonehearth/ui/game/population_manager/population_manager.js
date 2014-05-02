App.StonehearthCitizenManagerView = App.View.extend({
	templateName: 'citizenManager',

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:profession" : {
               "profession_uri" : {}
            },
            "unit_info": {},
         }
      }
   },

   init: function() {
      var self = this;
      this._super();
      radiant.call('stonehearth:get_population')
         .done(function(response){
            var uri = response.population;
            console.log('population uri is', uri);
            self.set('uri', uri);
         });
   },

   actions: {

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
    }.observes('context.citizens.[]'),

    _foo: function() {
      console.log('array size changed');
   }.observes('context.citizens.[]'),

    _foo2: function() {
      var arr = this.get('context.citizensArray');
      console.log('context.citizensArray.@each');
   }.observes('context.citizensArray.@each.stonehearth:profession.profession_uri.name'),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      this._super();
   },
});

App.StonehearthCrafterManagerView = App.View.extend({
   templateName: 'crafterManager',

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
      radiant.call('stonehearth:get_population')
         .done(function(response){
            var uri = response.population;
            console.log('population uri is', uri);
            self.set('uri', uri);
         });
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
    }.observes('context.citizens.[]'),

    _onCitizenCrafterComponentChanged: function() {
      var arr = this.get('context.citizensArray');

      this.set('context.citizensArray', arr);
    }.observes('context.citizensArray.@each.stonehearth:crafter'),

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      this._super();
   },
});
