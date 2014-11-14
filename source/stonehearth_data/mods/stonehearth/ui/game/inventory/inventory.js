
App.StonehearthInventoryView = App.View.extend({
   templateName: 'inventory',
   classNames: ['flex'],
   
   init: function() {
      var self = this;
      this._super();
   },

   _grabInventory: function() {
      var self = this;

      return radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', 'stonehearth:basic_inventory_tracker')
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

      // grab the data and sort it
      var arr = radiant.map_to_array(data);
      
      arr.sort(function(a, b){
         return a.display_name - b.display_name
      });

      // mark all items as not updated
      if (this.$('.item')) {
         this.$('.item').attr('updated', 0);   
      }

      // run through the array and add/update item divs as necessary
      var palette = this.$('.itemPalette');
      $.each(arr, function(i, item) {
         var itemElement = self._findElementForItem(item)
         
         if (!itemElement) {
            itemElement = self._addItemElement(item);
            palette.append(itemElement);
         } else {
            self._updateItemElement(itemElement, item);
         }

         itemElement.attr('updated', 1)
      })

      // anything that is not marked as updated needs to be removed
      palette.find('[updated=0]').remove();

      self.$('.item').tooltipster();
   }.observes('inventory'),

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
         .html(item.count)

      var itemEl = $('<div>')
         .addClass('item')
         .attr('title', item.display_name)
         .attr('uri', item.uri)
         .append(img)
         .append(num)

      return itemEl;
   },

   _updateItemElement: function(itemEl, item) {
      itemEl.find('.num').html(item.count)
   },

   didInsertElement: function() {
      this._super();
      this._grabInventory();
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
   },

});
