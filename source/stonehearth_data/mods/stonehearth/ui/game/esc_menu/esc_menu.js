
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

   _saveGame: function() {
      var d = new Date();
      var gameDate = App.gameView.getDate().date 
      var gameTime = App.gameView.getDate().time;

      var saveid = String(d.getTime());

      return radiant.call("radiant:client:save_game", saveid, { 
            name: '',
            town_name: App.stonehearthClient.settlementName(),
            game_date: gameDate,
            game_time: gameTime,
            timestamp: d.getTime(),
            time: d.toLocaleString(),
            jobs: {
               crafters: App.population.getNumCrafters(),
               workers: App.population.getNumWorkers(),
               soldiers: App.population.getNumSoldiers(),
            }
         });
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
         var self = this;
         var doSaveAndExit = function() {
            self._saveGame()
               .done(function() {
                  radiant.call('radiant:exit');
               });
         };

         var doJustExit = function() {
            radiant.call('radiant:exit');
         };

         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : i18n.t('stonehearth:esc_menu.exit_confirm_dialog.title'),
               message : i18n.t('stonehearth:esc_menu.exit_confirm_dialog.message'), 
               buttons : [
                  { 
                     id: 'saveAndExit',
                     label: i18n.t('stonehearth:esc_menu.exit_confirm_dialog.save_and_exit_button'),
                     click: doSaveAndExit
                  },
                  {
                     id: 'justExit',
                     label: i18n.t('stonehearth:esc_menu.exit_confirm_dialog.just_exit_button'),
                     click: doJustExit
                  },
                  {
                     id: 'cancel',
                     label: i18n.t('stonehearth:esc_menu.exit_confirm_dialog.cancel_button')
                  }
               ] 
            });

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
