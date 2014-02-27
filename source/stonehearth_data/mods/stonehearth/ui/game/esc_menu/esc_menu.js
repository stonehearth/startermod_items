$(document).ready(function(){
   $(document).keyup(function(e) {
      if(e.keyCode == 27 && !App.escMenu){
         App.escMenu = App.gameView.addView(App.StonehearthEscMenuView)
      } else {
         App.escMenu.destroy();
      }
   });
});


App.StonehearthEscMenuView = App.View.extend({
   templateName: 'escMenu',
   classNames: ['flex'],
   modal: true,

   init: function() {
      this._super();
   },

   destroy: function() {
      App.escMenu = null;
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
      qyuitToMainMenu: function() {

      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      settings: function() {
         App.gameView.addView(App.StonehearthSettingsView);
      }

   }

});
