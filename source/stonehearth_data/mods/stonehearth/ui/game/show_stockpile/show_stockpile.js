$(document).ready(function(){
   // When we get the show_workshop event, toggle the crafting window
   // for this entity.
   $(top).on("show_stockpile.stonehearth", function (_, e) {
      var view = App.gameView.addView(App.StonehearthShowStockpileView, { uri: e.entity });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthShowStockpileView = App.View.extend({
   templateName: 'stonehearthShowStockpile',

   components: {
      "unit_info": {},
      "stonehearth:stockpile" : {}
   },

   modal: true,

   groups : {
      resources : [
         "wood",
         "stone",
         "ore",
         "animal_part"
      ],
      construction : [
         "portal",
         "furniture",
         "defense",
         "light",
         "decoration"
      ],
      goods : [
         "refined_cloth",
         "refined_animal_part",
         "refined_ore"
      ],
      gear : [
         "melee_weapon",
         "ranged_weapon",
         "light_armor",
         "heavy_armor",
         "exotic_gear"
      ],
      food_and_drink : [
         "fruit",
         "baked",
         "meat",
         "drink"
      ],
      wealth : [
         "gold",
         "gem"
      ]
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      this.taxonomyGrid = $('#taxonomyGrid');
      this.allNoneGrid = $('#allNoneGrid');
      this.items = this.taxonomyGrid.find('.category');

      $( '#allNoneGrid' ).togglegrid({
         radios: true,
         onItemClick : function( el, ev ) { 
            self._toggleAllNone(el);
         }
      });

      this.taxonomyGrid.togglegrid();

      this.taxonomyGrid.find('.group').click(function() {
         self._toggleGroup($(this))
      });

      this.items.click(function() {
         self._itemClick($(this));
      });
      
      $('#stockpileWindow').find('#name').keypress(function (e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val())
        }
      });

      this.items.find('img').tooltip();

      self._refeshGrids();
   },

   tooltipText : function(category) {
      return("tt for " + category);
   }.property(),

   _toggleAllNone : function(element) {
      if (element.attr('id') == 'all') {
         // do the on thing
         this.taxonomyGrid.find('#all').click();
         this.taxonomyGrid.find('.toggleButton').addClass('on');
      } else {
         // do the off thing
         this.taxonomyGrid.find('#none').click();
         this.taxonomyGrid.find('.toggleButton').removeClass('on');
      }

      this._setStockpileFilter();
   },

   _refeshGrids : function() {
      if (!this.items) {
         return;
      }

      var self = this;

      console.log('refreshing stockpile grids!');
      var filter = this.get('context.stonehearth:stockpile.filter');

      if (filter) {
         this.items.removeClass('on');

         $.each(filter, function(name, value) {
            var element = self.taxonomyGrid.find('#' + name);
            if (value) {
               element.addClass('on');
            }
         });

         this._updateGroups();
      }

   }.observes('context.stonehearth:stockpile.filter'),

   _toggleGroup : function(element) {
      var on = element.hasClass('on');
      group = this.groups[element.attr('id')];
      $.each(group, function(i, category) {
         var button = $('#stockpileWindow').find('#' + category);
         
         if (button.length > 0) {
            if(on) {
               button.addClass('on');
            } else {
               button.removeClass('on');
            }
         }
      });

      this._setStockpileFilter();
   },

   _itemClick : function(element) {
      this._setStockpileFilter();
   },

   _setStockpileFilter : function() {
      var values = []
      this.items.each(function(i, item) {
         if($(item).hasClass('on')) {
            values.push($(item).attr('id'));
         }
      });

      radiant.call('stonehearth:set_stockpile_filter', this.uri, values)
         .done(function(response) {
            console.log(response)
         });
   },

   _updateGroups : function() {
      var self = this;

      var allGroupsOn = true;
      var noGroupsOn = true;

      $.each(self.groups, function(groupName, group) {
         var allOn = true;

         $.each(group, function(i, category) {
            if(!self.taxonomyGrid.find('#' + category).hasClass('on')) {
               allOn = false;
               allGroupsOn = false;
            } else {
               noGroupsOn = false;
            }
         });

         if (allOn) {
            self.taxonomyGrid.find('#' + groupName).addClass('on');
         } else {
            self.taxonomyGrid.find('#' + groupName).removeClass('on');
         }         

      });

      if (allGroupsOn) {
         self.allNoneGrid.find('#all').addClass('on')
      } else {
         self.allNoneGrid.find('#all').removeClass('on')
      }

      if (noGroupsOn) {
         self.allNoneGrid.find('#none').addClass('on')
      } else {
         self.allNoneGrid.find('#none').removeClass('on')
      }
   }

});