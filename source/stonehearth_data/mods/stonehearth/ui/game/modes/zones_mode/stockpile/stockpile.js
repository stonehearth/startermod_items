App.StonehearthStockpileView = App.View.extend({
   templateName: 'stonehearthStockpile',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:stockpile" : {}
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      this.taxonomyGrid = this.$('#taxonomyGrid');
      this.allButton = this.$('#all');
      this.noneButton = this.$('#none');
      this.items = this.$('.category');
      this.groups = this.$('.group');

      this.taxonomyGrid.togglegrid();

      this.taxonomyGrid.find('.group').click(function() {
         self._onGroupClick($(this))
      });

      this.items.click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
         self._onItemClick($(this));
      });

      this.allButton.click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
         self._selectAll();
      });
      
      this.noneButton.click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
         self._selectNone();
      });

      this.$('#name').keypress(function(e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
         }
      });

      this.$('button.ok').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:submenu_select' );
         self.destroy();
      });

      this.$('button.warn').click(function() {
         radiant.call('stonehearth:destroy_entity', self.uri)
         self.destroy();
      });

      this.items.find('img').tooltipster();

      self._refreshGrids();
   },

   _selectAll : function() {
      this.items.addClass('on');
      this._setStockpileFilter();
   },

   _selectNone : function() {
      this.items.removeClass('on');
      this._setStockpileFilter();
   },

   _refreshGrids : function() {
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
         this.allButton.prop('checked', true)
         this.groups.prop('checked', true)
         this.items.addClass('on');
      }
   }.observes('context.stonehearth:stockpile.filter'),

   _onGroupClick : function(element) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
      var checked = element.prop('checked');

      element.parent().siblings().each(function(i, sibling) {
         var category = $(sibling);

         if (category.hasClass('category')) {
            if (checked) {
               category.addClass('on');
            } else {
               category.removeClass('on');
            }
         }
      });

      this._setStockpileFilter();
   },

   _onItemClick : function(element) {
      this._setStockpileFilter();
   },

   _setStockpileFilter : function() {
      var values = [];
      $('#taxonomyGrid').find('.on').each(function(i, element) {
         var value = $(element).attr('filter');
         if (value) {
            values.push(value);
         }
      });

      radiant.call('stonehearth:set_stockpile_filter', this.uri, values)
         .done(function(response) {
            console.log(response)
         });
   },

   // As of jQuery 1.6, we should be using .prop() instead of .attr() to get and set the checked property of checkboxes:
   // http://api.jquery.com/prop
   // http://stackoverflow.com/questions/5874652/prop-vs-attr/5876747#5876747
   _updateGroups : function() {
      var self = this;

      var allGroupsOn = true;
      var noGroupsOn = true;

      this.groups.each(function (i, group) {
         var groupOn = true;
         $(group).parent().siblings().each(function(i, sibling) {
            var category = $(sibling);

            if (category.hasClass('category')) {
               if (category.hasClass('on')) {
                  noGroupsOn = false;
               } else {
                  groupOn = false;
                  allGroupsOn = false;
               }
            }
         });

         $(group).prop('checked', groupOn);
      });

      self.allButton.prop('checked', allGroupsOn);
      self.noneButton.prop('checked', noGroupsOn);
   }
});
