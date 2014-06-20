
// Common functionality between the save and load views
App.StonehearthSaveLoadView = App.View.extend({

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

      this.$('.saveList').on( 'click', '.row', function() {
         console.log($(this).attr('key'));
      });
   },

   actions: {
      saveGame: function() {
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
               time: d.toLocaleString()
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

         App.gameView.addView(App.StonehearthConfirmView, 
            { 
               title : "Confirm Save Overwrite",
               message : "Do you really want to completely, unrevokably overwrite this save game?",
               buttons : [
                  { 
                     label: "Yes",
                     click: function() {
                        radiant.call("radiant:client:delete_save_game", key)
                        radiant.call("radiant:client:save_game", String(t), { 
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
         var self = this;
         var key = this.getListView().getSelectedKey();

         if (key) {
            // throw up a loading screen. when the game is loaded the browser is refreshed,
            // so we don't need to worry about removing the loading screen, ever.
            App.gameView.addView(App.StonehearthLoadingScreenView, { hideProgress: true });
            radiant.call("radiant:client:load_game", key)
               .always(function() {
                  App.gotoGame();
               });
         }
      },
   }

});


App.StonehearthSaveListView = App.View.extend({
   templateName: 'saveList',
   classNames: [],

   init: function() {
      this._super();
      var self = this;

      this.set('context', {});

      this.refresh();
   },

   refresh: function() {
      var self = this;
      var saveKey = App.stonehearthClient.gameState.saveKey;

      radiant.call("radiant:client:get_save_games")
         .done(function(json) {
            var vals = [];

            $.each(json, function(k ,v) {
               if(k != "__self" && json.hasOwnProperty(k)) {
                  v['key'] = k;

                  if (k == saveKey) {
                     v['current'] = true;
                  }

                  vals.push(v);
               }
            });

            // sort by creation time
            vals.sort(function(a, b){
               var keyA = a.key;
               var keyB = b.key;
               // Compare the 2 keys
               if(keyA < keyB) return 1;
               if(keyA > keyB) return -1;
               return 0;
            });

            self.set('context.saves', vals);
            Ember.run.scheduleOnce('afterRender', self, 'selectFirstItem')
         })      
   },

   getSelectedKey: function() {
      return this.$().find('.selected').attr('key');
   },

   selectFirstItem: function() {
      var firstItem = this.$('.saveList').find('.row')[0];
      
      if (firstItem) {
         $(firstItem).addClass('selected');
      }
      
   },

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$().on( 'click', '.row', function() {
         self.$().find('.row').removeClass('selected');
         $(this).addClass('selected');
      });
   },

   actions: {

      foo: function() {

      },
   }

});
