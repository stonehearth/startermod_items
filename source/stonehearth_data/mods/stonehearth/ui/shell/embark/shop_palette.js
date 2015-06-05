$.widget( "stonehearth.stonehearthEmbarkShopPalette", {
   options: {
      canSelect: function(item) {
         return true
      },
      click: function(item) {
         console.log('Clicked item: ' + item);
      },
   },

   _create: function() {
      var self = this;
      this.palette = $('<div>').addClass('itemList');

      this.palette.on( 'click', '.item', function() {
         var itemSelected = $(this);


         if (!itemSelected.hasClass('selected')) {

            if (self.options.canSelect && !self.options.canSelect(itemSelected)) {
               return false;
            }

            itemSelected.addClass('selected');
         } else {
            itemSelected.removeClass('selected');
         }
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
         
         if (self.options.click) {
            self.options.click(itemSelected);
         }
         return false;
      });

      this.element.append(this.palette);
   },

   updateItems: function(itemMap) {
      var self = this;

      // mark all items as not updated
      this.palette.find('.item').attr('updated', 0);   
      // grab the items in this category and sort them
      var arr = radiant.map_to_array(itemMap);

      radiant.each(arr, function(i, item) {
         var itemElement = self._findElementForItem(item)
         
         if (!itemElement) {
            itemElement = self._addItemElement(item);
            self.palette.append(itemElement);
         } else {
            itemElement.removeClass('selected');
         }
         //self._updateItemElement(itemElement, item);
         if (i % 2 == 0) {
            itemElement.addClass('row0');
         } else {
            itemElement.addClass('row1');
         }

         itemElement.attr('updated', 1)
         
      })

      // anything that is not marked as updated needs to be removed
      self.palette.find('[updated=0]').remove();
      self.palette.find('.item').tooltipster();
   },

   _findElementForItem: function(item) {
      var selector = "[uri='" + item.uri + "']";      
      var match = this.palette.find(selector)[0];

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

      var displayName = $('<div>')
         .addClass('displayName')
         .html(item.display_name);

      var cost = $('<div>')
         .addClass('cost')
         .html(item.cost + 'g');

      var soldTag = $('<div>')
         .addClass('soldTag');


      var itemEl = $('<div>')
         .addClass('item')
         .attr('title', item.display_name)
         .attr('uri', item.uri)
         .attr('cost', item.cost)
         .attr('isPet', item.isPet)
         .append(img)
         .append(displayName)
         .append(cost)
         .append(soldTag);

      return itemEl;
   }
});
