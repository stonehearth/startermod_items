
App.StonehearthInventoryTrackerView = App.View.extend({
   templateName: 'inventory',
   classNames: ['flex'],
   
   init: function() {
      var self = this;
      this._super();
   },

   _grabInventory: function() {
      var self = this;
      var tracker = this.get('tracker');

      if (this._trace) {
         return;
      }
      
      return radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', tracker)
         .done(function(response) {
            self._trace = new StonehearthDataTrace(response.tracker, {})
               .progress(function(data) {
                  self.set('inventory', data.tracking_data);
                  //self._updateInventory(data.tracking_data);
               });

            //self.set('uri', response.tracker);
         })
         .fail(function(response) {
            console.error(response);
         })     
   },

   foo: function() {
      var data = this.get('context.tracking_data');
      var arr = radiant.map_to_array(data);
      //this.set('items', arr);
   }.observes('context.tracking_data'),

   _inventoryDataChanged: function() {
      Ember.run.scheduleOnce('afterRender', this, '_updateInventory');
   }.observes('context.tracking_data'),

   _updateInventory: function() {
      var self = this;
      var data = self.get('inventory');

      // mark all items as not updated
      this.$('.item').attr('updated', 0);   
      this.$('.category').attr('update', 0);

      // for each category
      $.each(data, function(name, items) {
         var categoryElement = self._findCategory(name);

         // grab the items in this category and sort them
         var arr = radiant.map_to_array(items);
         
         arr.sort(function(a, b){
            return a.display_name - b.display_name
         });
         
         $.each(arr, function(i, item) {
            var itemElement = self._findElementForItem(item)
            
            if (!itemElement) {
               itemElement = self._addItemElement(item);
               categoryElement.append(itemElement);
            } else {
               self._updateItemElement(itemElement, item);
            }

            itemElement.attr('updated', 1)
         })
      });

      // anything that is not marked as updated needs to be removed
      self.$('.itemPalette').find('[updated=0]').remove();
      self.$('.item').tooltipster();
   }.observes('inventory'),

   _findCategory: function(name) {
      var selector = "[category='" + name + "']";      
      var match = this.$(selector)[0];

      if (match) {
         return $(match);
      } else {
         var palette = this.$('.itemPalette');

         // new title element for the category
         $('<h2>')
            .html(name)
            .appendTo(palette);

         // the category container element that items are inserted into
         return $('<div>')
            .addClass('downSection')
            .attr('category', name)
            .appendTo(palette)
      }
   },

   _findElementForItem: function(item) {
      var selector = "[uri='" + item.uri + "']";      
      //selector = selector.split(':').join('\\\\:'); //escape the colons
      
      var match = this.$(selector)[0];

      if (match) {
         return $(match);
      } else {
         return null;
      }
   },

   _addItemElement: function(item) {
      var img = $('<img>')
         .addClass('image')
         .attr('src', item.icon);

      var num = $('<div>')
         .addClass('num')
         .html(Object.keys(item.items).length)

      var itemEl = $('<div>')
         .addClass('item')
         .attr('title', item.display_name)
         .attr('uri', item.uri)
         .append(img)
         .append(num)

      return itemEl;
   },

   _updateItemElement: function(itemEl, item) {
      itemEl.find('.num').html(Object.keys(item.items).length)
   },

   didInsertElement: function() {
      this._super();

      if(this.get('tracker')) {
         this._grabInventory();
      }
      
      /*
      this.$('#searchInput').keyup(function (e) {
         var search = $(this).val();

         if (!search || search == '') {
            self.$('.item').show();
            self.$('.category').show();
         } else {
            // hide items that don't match the search
            self.$('.item').each(function(i, item) {
               var el = $(item);
               var itemName = el.attr('title').toLowerCase();

               if(itemName.indexOf(search) > -1) {
                  el.show();
               } else {
                  el.hide();
               }
            })

            self.$('.category').each(function(i, category) {
               var el = $(category)

               if (el.find('.item:visible').length > 0) {
                  el.show();
               } else {
                  el.hide();
               }
            })
         }
         
      });
      */
      
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
   },

});
