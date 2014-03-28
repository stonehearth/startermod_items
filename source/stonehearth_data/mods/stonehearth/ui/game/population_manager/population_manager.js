App.StonehearthPopulationManagerView = App.View.extend({
	templateName: 'populationManager',

   components: {
      "citizens" : {
         "stonehearth:profession" : {},
         "unit_info": {}
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

   //When we hover over a command button, show its tooltip
   didInsertElement: function() {
      this._super();
   },
});
