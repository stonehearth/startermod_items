$.widget( "stonehearth.stonehearthItemPalette", {
   
   options: {

      click: function(item) {
         console.log('Clicked item: ' + item);
      },

      cssClass: ''
   },

   _create: function() {
      var self = this;

      this.palette = $('<div>').addClass('itemPalette');


      this.palette.on( 'click', '.item', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
         self.palette.find('.item').removeClass('selected');
         $(this).addClass('selected');

         if (self.options.click) {
            self.options.click($(this));
         }
         return false;
      });

      this.element.append(this.palette);
   },

   updateItems: function(data) {
      var self = this;

      // mark all items as not updated
      this.palette.find('.item').attr('updated', 0);   
      this.palette.find('.category').attr('update', 0);

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
            }
            self._updateItemElement(itemElement, item);

            itemElement.attr('updated', 1)
         })
      });

      // anything that is not marked as updated needs to be removed
      self.palette.find('[updated=0]').remove();
      self.palette.find('.item').tooltipster();
   }.observes('inventory'),

   _findCategory: function(name) {
      var selector = "[category='" + name + "']";      
      var match = this.palette.find(selector)[0];

      if (match) {
         return $(match);
      } else {

         // new title element for the category
         $('<h2>')
            .html(name)
            .appendTo(this.palette);

         // the category container element that items are inserted into
         return $('<div>')
            .addClass('downSection')
            .attr('category', name)
            .appendTo(this.palette)
      }
   },

   _findElementForItem: function(item) {
      var selector = "[uri='" + item.uri + "']";      
      //selector = selector.split(':').join('\\\\:'); //escape the colons
      
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

      var num = $('<div>')
         .addClass('num');

      var selectBox = $('<div>')
         .addClass('selectBox');

      var itemEl = $('<div>')
         .addClass('item')
         .addClass(this.options.cssClass)
         .attr('title', item.display_name)
         .attr('uri', item.uri)
         .append(img)
         .append(num)
         .append(selectBox);

      if (this.options.itemAdded) {
         this.options.itemAdded(itemEl, item);
      }

      return itemEl;
   },

   _updateItemElement: function(itemEl, item) {
      var num = 0;

      if (item.items) {
         num = Object.keys(item.items).length
      } else {
         num = item.num;
      }

      if (num == 0) {
         itemEl.remove();
      } else {
         itemEl.find('.num').html(num);
      }
   },
});
