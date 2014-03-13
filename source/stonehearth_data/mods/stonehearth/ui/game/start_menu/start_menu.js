App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',
   classNames: ['flex', 'fullScreen'],

   components: {

   },

   menuActions: {
      create_stockpile: function () {
         App.stonehearthClient.createStockpile();
      },
      create_farm : function () {
         App.stonehearthClient.createFarm();
      },
      build_wall: function () {
         App.stonehearthClient.buildWall();
      },
      build_simple_room: function () {
         App.stonehearthClient.buildRoom();
      },
      placeItem: function () {
         $(top).trigger('radiant_show_placement_menu');
      }
   },

   didInsertElement: function() {
      var self = this;
      $('#startMenuTrigger').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:trigger_click' );
      });

      App.stonehearth.startMenu = self.$('#startMenu');

      /*
      radiant.call('stonehearth:get_start_menu_data')
         .done(function (obj) {
            radiant.trace(obj.datastore)
               .progress(function(result) {
                  self._buildMenu(result);
               })
               .fail(function(e) {
                  console.log(e);
               });
         });
      */

      $.get('/stonehearth/data/ui/start_menu.json')
         .done(function(json) {
            self._buildMenu(json);
         });

      /*
      $('#startMenu').on( 'mouseover', 'a', function() {
         radiant.call('radiant:play_sound', "stoneheardh:sounds:ui:action_hover");
      });

      $('#startMenu').on( 'mousedown', 'li', function() {
         radiant.call('radiant:play_sound', "stonehearth:sounds:ui:action_click");
      });
      */

   },

   _buildMenu : function(data) {
      var self = this;
      this.$('#startMenu').stonehearthMenu({ 
         data : data,
         click : function (id, nodeData) {
            self._onMenuClick(id, nodeData);
         }
      });

   },

   _onMenuClick: function(menuId, nodeData) {
      //radiant.keyboard.setFocus(this.$());
      var menuAction = this.menuActions[menuId];

      // do the menu action
      if (menuAction) {
         menuAction();
      }
   }

});

