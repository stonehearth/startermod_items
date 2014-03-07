App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',
   classNames: ['flex', 'fullScreen'],

   components: {

   },

   menuActions: {
      create_stockpile: function () {
         App.stonehearthClient.createStockpile();
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

      $.get('/stonehearth/data/ui/start_menu.json')
         .done(function(json) {
            App.stonehearth.startMenu = self.$('#startMenu');
            self.$('#startMenu').stonehearthMenu({ 
                  data : json,
                  click : function (id, nodeData) {
                     self._onMenuClick(id, nodeData);
                  }
               });


         });
      
      /*
      $( '#startMenu' ).dlmenu({
         animationClasses : { 
            classin : 'dl-animate-in-sh', 
            classout : 'dl-animate-out-sh' 
         },
         onLinkClick : function( el, ev ) { 
            var menuId = $(el).find("a").attr('menuId');
            self.onMenuClick(menuId)
         }
      });

      $('#startMenuTrigger').hover(
         function() {
            $(this).removeClass('startMenuHoverOut')
            $(this).addClass('startMenuHoverIn')
         }, function() {
            $(this).removeClass('startMenuHoverIn')
            $(this).addClass('startMenuHoverOut')
      });      

      $('#startMenu').on( 'mouseover', 'a', function() {
         radiant.call('radiant:play_sound', "stonehearth:sounds:ui:action_hover");
      });

      $('#startMenu').on( 'mousedown', 'li', function() {
         radiant.call('radiant:play_sound', "stonehearth:sounds:ui:action_click");
      });
      */

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

