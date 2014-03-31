
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
               name: "Save game name",
               town_name: "Town name goes here",
               game_date: gameDate,
               game_time: gameTime,
               time: d.toLocaleString()
            })
            .always(function() {
               self.refreshList();
            });
      },

      deleteSaveGame: function() {
         var self = this;
         var key = this.$('.saveList')
                       .find('.selected')
                       .attr('key');

         if (key) {
            radiant.call("radiant:client:delete_save_game", String(key))
               .always(function() {
                  self.refreshList();
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
            radiant.call("radiant:client:load_game", key)
               .always(function() {
                     self.destroy();
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

            // XXX, sort by time
            $.each(json, function(k ,v) {
               if(k != "__self" && json.hasOwnProperty(k)) {
                  v['key'] = k;

                  if (k == saveKey) {
                     v['current'] = true;
                  }

                  vals.push(v);
               }
            });

            self.set('context.saves', vals);
         })      
   },

   getSelectedKey: function() {
      return this.$().find('.selected').attr('key');
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
