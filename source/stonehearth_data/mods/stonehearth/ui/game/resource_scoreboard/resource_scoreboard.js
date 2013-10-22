App.StonehearthResourceScoreboardView = App.View.extend({
   templateName: 'resourceScoreboard',

   components: {
      'resource_types' : {

      }
   },

   init: function() {
      this._super();
      /*
      var self = this;
      radiant.call('stonehearth:get_resource_tracker')
         .done(function(response) {
            console.log(response.tracker);
            self.set('uri', response.tracker);
         });
      */
   },

   foo: function() {
      var ctx = this.get('context');
      var newData = this.get('context').resource_types

      var resources = []
      for (var entry in newData) {
          if (newData.hasOwnProperty(entry)) {
            resources.push({
               name: entry,
               count: newData[entry]
            })
          }
      }
      this.set('context.resources', resources);
   }.observes('context'),

});

