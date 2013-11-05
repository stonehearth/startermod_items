App.StonehearthNewGameView = App.View.extend({
   templateName: 'stonehearthNewGame',
   i18nNamespace: 'stonehearth',

   modal: true,

   init: function() {
      this._super();
      var self = this;

      radiant.call('radiant:get_collection_status')
            .done(function(o) {
               var collection_status = o.collection_status;
               self.set('context.collection_status', collection_status);
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
