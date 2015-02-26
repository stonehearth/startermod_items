
// Common functionality between the save and load views
App.StonehearthSaveLoadView = App.View.extend({

   didInsertElement: function() {
      this._super();

      this.$('.saveList').on( 'click', '.row', function() {        
         console.log($(this).attr('key'));
      });
   },

   getListView: function() {
      // a bit of a hack. grab the Ember View for the list from its element
      var viewEl = this.$('#saves').children()[0]
      var id = $(viewEl).attr('id');

      return Ember.View.views[id];
   },

   refreshList: function() {
      var list = this.getListView();
      list.refresh();
   }

});

App.StonehearthSaveView = App.StonehearthSaveLoadView.extend({
   templateName: 'saveView',
   classNames: ['flex'],
   modal: true,

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      this._super();
      this.$('#saveView').position({
            my: 'center center',
            at: 'center center-150',
            of: '#modalOverlay'
         });
   },

   actions: {
      saveGame: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
         var self = this;
         var d = new Date();
         var t = d.getTime();
         var gameDate = App.gameView.getDate().date 
         var gameTime = App.gameView.getDate().time;

         radiant.call("radiant:client:save_game", String(t), { 
               name: "",
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
               self.refreshList();
            });
      },

       overwriteSaveGame: function() {
         var self = this;
         var d = new Date();
         var t = d.getTime();
         var gameDate = App.gameView.getDate().date 
         var gameTime = App.gameView.getDate().time;
         var key = String(this.getListView().getSelectedKey());

         if (!key || key == '') {
            return
         }
         
         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : "Confirm Save Overwrite",
               message : "Do you really want to completely, unrevokably overwrite this save game?",
               buttons : [
                  { 
                     label: "Yes",
                     click: function() {
                        radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
                        radiant.call("radiant:client:save_game", key, { 
                              name: "",
                              town_name: App.stonehearthClient.settlementName(),
                              game_date: gameDate,
                              game_time: gameTime,
                              time: d.toLocaleString()
                           })
                           .always(function() {
                              self.refreshList();
                           });
                     }
                  },
                  {
                     label: "No!"
                  }
               ] 
            });
      },
      deleteSaveGame: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
         var self = this;
         var key = this.getListView().getSelectedKey();

         if (key) {
            App.gameView.addView(App.StonehearthConfirmView, 
               { 
                  title : "Confirm Delete",
                  message : "Do you really want to delete this save game?",
                  buttons : [
                     { 
                        label: "Yes",
                        click: function() {
                           radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:trash' });
                           radiant.call("radiant:client:delete_save_game", String(key))
                              .always(function() {
                                 self.refreshList();
                              });
                        }
                     },
                     {
                        label: "No!"
                     }
                  ] 
               });
         }
      }
   }

});


App.StonehearthLoadView = App.StonehearthSaveLoadView.extend({
   templateName: 'loadView',
   classNames: ['flex'],
   modal: true,

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      this._super();
      this.$('#loadView').position({
            my: 'center center',
            at: 'center center-150',
            of: '#modalOverlay'
         });
   },

   actions: {
      loadGame: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:small_click' });
         var self = this;
         var key = this.getListView().getSelectedKey();

         if (key) {
            radiant.call("radiant:client:load_game", key);
            // At this point, we just wait to be killed by the client.
         }
      },
   }

});


App.StonehearthSaveListView = App.View.extend({
   templateName: 'saveList',
   classNames: [],

   _updateSelectedScreenshot: function() {
      var self = this;
      var currentScreenshot = undefined;

      var selected = this.$('.selected');
      if (selected) {
         var key = selected.attr('key');
         var saved_games = self.get('saved_games');
         if (saved_games && key) {
            currentScreenshot = saved_games[key].screenshot;
         }
      }
      self.set('currentScreenshot', currentScreenshot);
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$().on( 'click', '.row', function() {
         self.$().find('.row').removeClass('selected');
         $(this).addClass('selected');
         self._updateSelectedScreenshot();
      });
      this.refresh();
   },
 
   refresh: function() {
      var self = this;
      radiant.call("radiant:client:get_save_games")
               .done(function(json) {
                  self.set('saved_games', json);
                  
                  // select the first save after the view re-renderes.
                  Ember.run.scheduleOnce('afterRender', this, function() {
                     var rows = self.$('.saveList .row');

                     if (rows.length > 0) {
                        $(rows[0]).click()
                     }
                  });
               });
   },

   saves: function() {
      var saveKey = App.stonehearthClient.gameState.saveKey;
      var vals = radiant.map_to_array(this.get('saved_games'), function(k ,v) {
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
         var keyA = a.timestamp ? a.timestamp : a.key;
         var keyB = b.timestamp ? b.timestamp : b.key;
         // Compare the 2 keys
         if(keyA < keyB) return 1;
         if(keyA > keyB) return -1;
         return 0;
      });

      return vals;
   }.property('saved_games'),

   getSelectedKey: function() {
      return this.$().find('.selected').attr('key');
   },

});
