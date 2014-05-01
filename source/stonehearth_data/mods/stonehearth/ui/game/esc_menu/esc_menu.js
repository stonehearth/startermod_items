
App.StonehearthEscMenuView = App.View.extend({
   templateName: 'escMenu',
   classNames: ['flex'],
modal: true,

   init: function() {
      this._super();

      //Setting gamespeed to zero
      radiant.call('radiant:game:set_game_speed', 0);
   },

   didInsertElement: function() {
      this.$('#escMenu').position({
            my: 'center center',
            at: 'center center',
            of: '#modalOverlay'
         });
   },

   destroy: function() {
      this._super();
      var game_speed =  App.stonehearthClient.defaultGameSpeed();
      if (App.stonehearthClient.getPaused()) {
         game_speed = 0;
      } 
      radiant.call('radiant:game:set_game_speed', game_speed);
   },

   actions: {

      resume: function() {
         this.destroy();
      },

      qyuitToMainMenu: function() {

      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      settings: function() {
         App.gameView.addView(App.StonehearthSettingsView);
      },

      save: function() {
         App.gameView.addView(App.StonehearthSaveView);
      },

      load: function() {
         App.gameView.addView(App.StonehearthLoadView);
      }

   }

});
