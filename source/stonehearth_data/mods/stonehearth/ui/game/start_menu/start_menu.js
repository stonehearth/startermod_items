App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',

   components: {

   },

   menuActions: {
      createStockpile: {
         click: function () {
            $(top).trigger('radiant_create_stockpile');
         }
      },
      buildCarpenterWorkshop: {
         click: function () {
            $(top).trigger('build_workshop.stonehearth', {
               uri: '/stonehearth/entities/professions/carpenter/profession_description.json'
            });
         }
      },
      buildWallLoop: {
         click: function () {
            $(top).trigger('radiant_create_wall');
         }         
      },
      buildRoom: {
         click: function () {
            $(top).trigger('radiant_create_room');
         }         
      },
      placeItem: {
         click: function () {
            $(top).trigger('radiant_show_placement_menu');
         }         
      }
   },

   init: function() {
      this._super();
      // xxx, grab the initial button state from some server-side component
   },

   didInsertElement: function() {
      var self = this;
      $('#startMenuTrigger').click(function() {
         //radiant.call('radiant:play_sound', 'stonehearth:sounds:axe_chopping_sound' );
      });

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
   },

   onMenuClick: function(menuId) {
      this.menuActions[menuId].click();
   }  

});

