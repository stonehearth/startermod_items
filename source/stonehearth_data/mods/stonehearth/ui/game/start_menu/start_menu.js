App.StonehearthStartMenuView = App.View.extend({
   templateName: 'stonehearthStartMenu',

   components: {

   },

   menuActions: {
      createStockpile: {
         click: function () {
            $(top).trigger('create_stockpile.radiant');
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
            $(top).trigger('create_wall.radiant');
         }         
      },
      buildRoom: {
         click: function () {
            $(top).trigger('create_room.radiant');
         }         
      },
      placeItem: {
         click: function () {
            $(top).trigger('placement_menu.radiant');
         }         
      }
   },

   init: function() {
      this._super();
      // xxx, grab the initial button state from some server-side component
   },

   didInsertElement: function() {
      var self = this;
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
   },

   onMenuClick: function(menuId) {
      this.menuActions[menuId].click();
   }  

});

