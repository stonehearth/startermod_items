$(document).ready(function(){
   // When we get the show_workshop event, toggle the crafting window
   // for this entity.
   $(top).on("radiant_show_stockpile", function (_, e) {
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

   didInsertElement: function() {
      this._super();
      var self = this;
      this.taxonomyGrid = $('#taxonomyGrid');
      this.allNoneGrid = $('#allNoneGrid');
      this.allButton = this.allNoneGrid.find('#all');
      this.noneButton = this.allNoneGrid.find('#none');
      this.items = this.taxonomyGrid.find('.category');
      this.groups = this.taxonomyGrid.find('.group');

      $( '#allNoneGrid' ).togglegrid({
         radios: true
      });

      this.taxonomyGrid.togglegrid();

      this.taxonomyGrid.find('.group').click(function() {
         self._toggleGroup($(this))
      });

      this.items.click(function() {
         self._itemClick($(this));
      });

      this.allButton.click(function() {
         self._selectAll();
      });
      
      this.noneButton.click(function() {
         self._selectNone();
      });

      this.$('#name').focus(function (e) {
         radiant.call('stonehearth:enable_camera_movement', false)
      });

      this.$('#name').blur(function (e) {
         radiant.call('stonehearth:enable_camera_movement', true)
      });

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
        }
      });

      this.items.find('img').tooltipster();

      self._refeshGrids();
   },

   _selectAll : function() {
      //this.taxonomyGrid.find('#all').click();
      this.items.addClass('on');
      this._setStockpileFilter();
   },

   _selectNone : function() {
      //this.taxonomyGrid.find('#none').click();
      this.items.removeClass('on');
      this._setStockpileFilter();
   },

   _refeshGrids : function() {
      if (!this.items) {
         return;
      }

      var self = this;
      var stockpileFilter = this.get('context.stonehearth:stockpile.filter');

      $('.category').removeClass('on');

      if (stockpileFilter) {
         this.items.each(function(i, element) {
            var button = $(element)
            var buttonFilter = button.attr('filter')

            if (!buttonFilter) {
               console.log('button ' + button.attr('id') + " has no filter!")
            } else {
               $.each(stockpileFilter, function(i, filter) {
                  if (buttonFilter == filter) {
                     button.addClass('on');
                  }
               });
            }

         });

         this._updateGroups();
      } else {
         this.allButton.addClass('on');
      }

   }.observes('context.stonehearth:stockpile.filter'),

   _toggleGroup : function(element) {
      var on = element.hasClass('on');

      element.siblings().each(function(i, sibling) {
         var category = $(sibling);

         if (category.hasClass('category')) {
            if (on) {
               category.addClass('on');
            } else {
               category.removeClass('on');
            }
         }
      });

      this._setStockpileFilter();
   },

   _itemClick : function(element) {
      this._setStockpileFilter();
   },

   _setStockpileFilter : function() {
      var values = [];
      $('#taxonomyGrid').find('.on').each(function(i, element) {
         if ($(element).attr('filter')) {
            values.push($(element).attr('filter'));
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

      this.groups.each(function (i, group) {
         var allOn = true;
         $(group).siblings().each(function(i, sibling) {
            var category = $(sibling);

            if (category.hasClass('category')) {
               if (!category.hasClass('on')) {
                  allOn = false;
                  allGroupsOn = false;
               } else {
                  noGroupsOn = false;
               }
            }
         });

         if (allOn) {
            $(group).addClass('on');
         } else {
            $(group).removeClass('on');
         }         

      });

      if (allGroupsOn) {
         self.allButton.addClass('on')
         self.items.removeClass('on');
         self.groups.removeClass('on');
      } else {
         self.allButton.removeClass('on')
      }

      if (noGroupsOn) {
         self.noneButton.addClass('on')
         self.items.removeClass('on');
         self.groups.removeClass('on');
      } else {
         self.noneButton.removeClass('on')
      }
   }

});