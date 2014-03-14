App.StonehearthFarmView = App.View.extend({
   templateName: 'stonehearthFarm',

   components: {
      "unit_info": {},
      "stonehearth:stockpile" : {}
   },


   init: function() {
      this._super();
      this.set('foo', 'hello world');

      this.set('crop_rotation', [
         {
            crop: 'corn',
         },
         {
            crop: 'turnips',
         }         
      ]);
   },

   didInsertElement: function() {
      this._super();
      var self = this;

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

      this.$('button.warn').click(function() {
         self.destroy();
      });

      this.$('#farmWindow').draggable();

   },

   destroy : function() {
      radiant.call('stonehearth:enable_camera_movement', true);
      this._super();
   },

   actions :  {
      addCrop: function() {
         palette = App.gameView.addView(App.StonehearthFarmCropPalette, { 
                     context: {
                        items : [
                           {
                              icon: "..."
                           },
                           {
                              icon: "..."
                           }
                        ]
                     },
                     position: {
                        my : 'center',
                        at : 'center',
                        of : this.$('#addCropButton')
                     }
                  });
      },
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
         this.allButton.attr('checked', 'checked');
         this.groups.attr('checked', 'checked');
         this.items.addClass('on');
      }

   }.observes('context.stonehearth:stockpile.filter'),

   _toggleGroup : function(element) {
      var foo = element.attr('checked');
      var on = !(element.attr('checked') == 'checked');

      element.parent().siblings().each(function(i, sibling) {
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
         $(group).parent().siblings().each(function(i, sibling) {
            var category = $(sibling);

            if (category.hasClass('category')) {
               if (category.hasClass('on')) {
                  noGroupsOn = false;
               } else {
                  allOn = false;
                  allGroupsOn = false;
               }
            }
         });

         if (allOn) {
            $(group).attr('checked', 'checked')
         } else {
            $(group).removeAttr('checked')
         }         

      });

      if (allGroupsOn) {
         self.allButton.attr('checked', 'checked')
         //self.items.removeClass('on');
         //self.groups.removeClass('on');
      } else {
         self.allButton.removeAttr('checked')
      }

      if (noGroupsOn) {
         self.noneButton.attr('checked', 'checked')
         //self.items.removeClass('on');
         //self.groups.removeClass('on');
      } else {
         self.noneButton.removeAttr('checked')
      }
   }

});

App.StonehearthFarmCropPalette = App.View.extend({
   templateName: 'stonehearthCropPalette',
   modal: true,

});