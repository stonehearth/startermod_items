App.StonehearthNewGameView = App.View.extend({
   templateName: 'stonehearthNewGame',
   i18nNamespace: 'stonehearth',

   modal: true,

   init: function() {
      this._super();
      var self = this;

      radiant.call('radiant:get_collection_status')
            .done(function(o) {
               var expressed_opinion = o.has_expressed_preference;
               var collection_status = o.collection_status;
               if (expressed_opinion) {
                  self.set('context.collection_status', collection_status);
               } else {
                  //User hasn't expressed an opnion yet, so default to yes
                  self.set('context.collection_status', true);
               }
            });  
   },

   actions : {
      startNewGame: function() {
         var self = this;
         radiant.call('stonehearth:new_game')
            .done(function(o) {
               console.log('stonehearth:newgame came back with result: ' + o);
               App.gotoGame();
               self.buildStartingCamp();
            });         

         radiant.call('radiant:set_collection_status', $('#analyticsCheckbox').is(':checked'))
            .done(function(o) {
               console.log('radiant:set_collection_status came back with result: ' + o);
            });         

      }
   },

   buildStartingCamp: function() {
      App.gameView.addView(App.StonehearthCreateCampView)
      this.destroy();
   }
});
