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
        var attributeMap = this.get('context.citizens');
        
        if (attributeMap) {
           $.each(attributeMap, function(k ,v) {
              if(k != "__self" && attributeMap.hasOwnProperty(k)) {
                 vals.push(v);
              }
           });
        }

        this.set('context.citizensArray', vals);
    }.observes('context.citizens'),

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

   _buildCraftersArray: function() {
        var vals = [];
        var attributeMap = this.get('context.citizens');
        
        if (attributeMap) {
           $.each(attributeMap, function(k ,v) {
              if(k != "__self" && attributeMap.hasOwnProperty(k)) {
                  //xxx insert a check to see if this dude is a crafter
                 vals.push(v);
              }
           });
        }

        this.set('context.craftersArray', vals);
    }.observes('context.citizens'),


   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      this._super();
   },
});
