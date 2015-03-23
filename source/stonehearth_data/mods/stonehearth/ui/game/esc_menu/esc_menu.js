
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
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:pause' });
      this.$('#escMenu').position({
            my: 'center center',
            at: 'center center',
            of: '#modalOverlay'
         });
   },

   destroy: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:resume' });
      this._super();
      radiant.call('stonehearth:dm_resume_game');
   },

   actions: {

      resume: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:resume' });
         this.destroy();
      },

      quitToMainMenu: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });

      },

      exit: function() {
         radiant.call('radiant:exit');
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
      },

      settings: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
         App.gameView.addView(App.StonehearthSettingsView);
      },

      postBug: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
         App.gameView.addView(App.StonehearthPostBugView);
      },
   }

});
