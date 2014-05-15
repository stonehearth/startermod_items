
App.StonehearthEscMenuView = App.View.extend({
   templateName: 'escMenu',
   classNames: ['flex'],
   modal: true,

   init: function() {
      this._super();

      //Setting gamespeed to zero
      radiant.call('stonehearth:dm_pause_game');
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
      radiant.call('stonehearth:dm_resume_game');
   },

   actions: {

      resume: function() {
         this.destroy();
      },

      quitToMainMenu: function() {

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
