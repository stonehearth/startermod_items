
App.StonehearthEscMenuView = App.View.extend({
   templateName: 'escMenu',
   classNames: ['flex'],
modal: true,

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      this.$('#escMenu').position({
            my: 'center center',
            at: 'center center',
            of: '#modalOverlay'
         });
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
