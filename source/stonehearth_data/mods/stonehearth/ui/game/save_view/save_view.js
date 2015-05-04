// This is the controller. It is responsible for two things.
// 1) Managing the data that is provided to the view. Any big transforms should be done in the controller, not the view
// 2) Publishing an actions API for use by the view and other controllers.

App.SaveController = Ember.Controller.extend(Ember.ViewTargetActionSupport, {

   _doInit: function() {
      this._super();
      this._getSaves();
      this._getAutoSaveSetting();
   }.on('init'),

   _foo: function() {
      console.log(this.get(model));
   }.on('didInsertElement'),

   // grab all the saves from the server
   _getSaves: function() {
      var self = this;
      radiant.call("radiant:client:get_save_games")
         .done(function(json) {
            var formattedSaves = self._formatSaves(json);
            self.set('saves', formattedSaves);
         });
   },

   // reformat the save map into an array sorted by time, for the view to consume
   _formatSaves: function(saves) {
      var saveKey = App.stonehearthClient.gameState.saveKey;
      var vals = radiant.map_to_array(saves, function(k ,v) {
         v.key = k;
         if (k == saveKey) {
            v.current = true;
         }
         if (!v.gameinfo.version || v.gameinfo.version != App.stonehearthVersion) {
            // For now, just blindly warn if versions are different.
            v.differentVersions = true;
         }
      });

      // sort by creation time
      vals.sort(function(a, b){
         var keyA = a.gameinfo.timestamp ? a.gameinfo.timestamp : a.key;
         var keyB = b.gameinfo.timestamp ? b.gameinfo.timestamp : b.key;
         // Compare the 2 keys
         if(keyA < keyB) return 1;
         if(keyA > keyB) return -1;
         return 0;
      });

      return vals;
   },

   _getAutoSaveSetting: function() {
      var self = this;

      radiant.call('radiant:get_config', 'enable_auto_save')
         .done(function(response) {
            self.set('auto_save', response.enable_auto_save == true);
         })
   },

   _toggleAutoSave: function() {
      // XXX, this pattern is a little weird, but it lets us
      // 1. use dual-binding between a checkbox and the data in the controller
      // 2. have a clean API in actions
      var enabled = this.get('auto_save');
      this.send('enableAutoSave', enabled);
   }.observes('auto_save'),

   // the action interface
   actions: {
      saveGame: function(saveid) {
         var self = this;
         var d = new Date();
         var gameDate = App.gameView.getDate().date 
         var gameTime = App.gameView.getDate().time;

         if (!saveid) {
            saveid = String(d.getTime());
         }

         self.set('opInProgress', true);


         // show the "saving.... message"
         self.triggerAction({
            action:'openInOutlet',               
            actionContext: {
               viewName: 'savePopup',   
               outletName: 'modalmodal'
            }
         });

         radiant.call("radiant:client:save_game", saveid, { 
               name: saveid == 'auto_save' ? i18n.t('stonehearth:auto_save_prefix') : '',
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
            })
            .always(function() {
               self.set('opInProgress', false);
               self._getSaves();

               // hide the "saving.... message"
               self.triggerAction({
                  action:'closeOutlet',               
                  actionContext: {
                     outletName: 'modalmodal'
                  }
               });               
            });
      },

      loadGame: function(key) {
         radiant.call("radiant:client:load_game", key);         
      },

      deleteSaveGame: function(key) {
         var self = this;

         if (key) {
            self.set('opInProgress', true);
            radiant.call("radiant:client:delete_save_game", String(key))
               .always(function() {
                  self._getSaves();
                  self.set('opInProgress', false);
               });
         }
      },

      enableAutoSave: function(enable) {
         radiant.call('radiant:set_config', 'enable_auto_save', enable);
      }
   },
});

// This is the view. It has two jobs
// 1) Render to the screen through the template
// 2) Handle user interaction and invoke the controller
App.SaveView = App.View.extend(Ember.ViewTargetActionSupport, {
   classNames: ['flex'],
   modal: true,

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      this._super();
      /*
      this.$('#saveView').position({
            my: 'center center',
            at: 'center center-150',
            of: '#modalOverlay'
         });
      */
   },

   // when a controller op is in progress (saving, etc), disable the buttons
   enableButtons : function(enabled) {
      //XXX use bind-attr to do this
      var enabled = !this.get('controller.opInProgress');

      if (this.$()) {
         if (enabled) {
            this.$('#deleteSaveButton').removeClass('disabled')
            this.$('#loadSaveButton').removeClass('disabled')
            this.$('#overwriteSaveButton').removeClass('disabled')
            this.$('#createSaveButton').removeClass('disabled')
         } else {
            this.$('#deleteSaveButton').addClass('disabled')
            this.$('#loadSaveButton').addClass('disabled')
            this.$('#overwriteSaveButton').addClass('disabled')
            this.$('#createSaveButton').addClass('disabled')
         }
      }
   }.observes('controller.opInProgress'),

   // when the array of saves is updated, select the first save
   _selectFirstSave: function() {
      var saves = this.get('controller.saves');
      if (saves) {
         this.set('selectedSave', saves[0]);   
      }
   }.observes('controller.saves'),

   // when the user selects a new save, manipulate the css classes so it highlights in the view
   _updateSelection: function() {
      Ember.run.scheduleOnce('afterRender', this, function() {
         // Update the UI. XXX, is there a way to do this without jquery?
         var key = this.get('selectedSave.key');

         this.$('.saveSlot').removeClass('selected');
         
         if (key) {
            this.$('[key=' + key + ']').addClass('selected');
         }
      });
      
   }.observes('selectedSave'),

   actions: {
      selectSave: function(save) {
         if (save) {
            this.set('selectedSave', save)
         }
      },

      saveGame: function() {
         // isn't there a more Ember-y way to do this?
         if (this.$('#deleteSaveButton').hasClass('disabled')) {
            return;
         }
         this.get("controller").send('saveGame');
      },

      loadGame: function() {
         // isn't there a more Ember-y way to do this?
         if (this.$('#deleteSaveButton').hasClass('disabled')) {
            return;
         }
         var key = this.get('selectedSave.key');

         if (key) {
            this.get("controller").send('loadGame', key);
         }
         
      },

      overwriteSaveGame: function() {
         //XXX, need to handle validation in an ember-friendly way. No jquery
         if (this.$('#overwriteSaveButton').hasClass('disabled')) {
            return;
         }

         var self = this;
         var key = this.get('selectedSave.key');

         if (!key || key == '') {
            return;
         }
         
         // open the confirmation screen
         self.triggerAction({
            action:'openInOutlet',               
            actionContext: {
               viewName: 'confirm',   
               outletName: 'modalmodal',
               controller: {
                  title: i18n.t('stonehearth:save_confim_overwrite_title'),
                  message: i18n.t('stonehearth:save_confirm_overwrite_message'),
                  buttons: [
                     { 
                        label: i18n.t('stonehearth:yes'),
                        click: function() {
                           radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash' });
                           self.get("controller").send('saveGame', key);
                        }
                     },
                     {
                        label: i18n.t('stonehearth:no')
                     }
                  ]
               }
            }
         });         
      },

      deleteSaveGame: function() {
         //XXX, need to handle validation in an ember-friendly way. No jquery
         if (this.$('#deleteSaveButton').hasClass('disabled')) {
            return;
         }
         
         var self = this;
         var key = this.get('selectedSave.key');

         if (!key || key == '') {
            return;
         }

         // open the confirmation screen
         self.triggerAction({
            action:'openInOutlet',               
            actionContext: {
               viewName: 'confirm',   
               outletName: 'modalmodal',
               controller: {
                  title: i18n.t('stonehearth:save_confim_delete_title'),
                  message: i18n.t('stonehearth:save_confirm_delete_message'),
                  buttons: [
                     { 
                        label: i18n.t('stonehearth:yes'),
                        click: function() {
                           radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash' });
                           self.get("controller").send('deleteSaveGame', key);
                        }
                     },
                     {
                        label: i18n.t('stonehearth:no')
                     }
                  ]
               }
            }
         });
      }
   }
});
