App.StonehearthTitleScreenView = App.View.extend({
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth',
   components: {},

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      radiant.call('radiant:get_collection_status')
         .done(function(o) {
            if (!o.has_expressed_preference) {
               self.get('parentView').addView(App.StonehearthAnalyticsOptView)
            }
         });  
   },

   actions: {
      newGame: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:embark' );
         App.shellView.addView(App.StonehearthLoadingScreenView);
         //this.get('parentView').addView(App.StonehearthNewGameView)
      },

      loadGame: function() {
         App.gotoGame();
      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      credits: function() {

      }
   }
});
