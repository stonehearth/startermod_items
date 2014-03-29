
App.StonehearthSaveView = App.View.extend({
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
         radiant.call("radiant:client:save_game", "slot_0", { 
               name: "Save game name",
               town_name: "Town name goes here",
               game_time: "game time goes here",
               time: 1337
            })
      },
   }

});


App.StonehearthLoadView = App.View.extend({
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
         radiant.call("radiant:client:load_game", "slot_0");
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


   actions: {

      foo: function() {

      },
   }

});
