$.widget( "stonehearth.stonehearthItemPalette", {
   
   options: {

      click: function(item) {
         console.log('Clicked item: ' + item);
      },

      filter: function(item) {
         return true;
      },

      cssClass: '',

      // If true, items with a num of 0 will be removed.
      // If false, items with a num of 0 will be disabled.
      removeZeroNumItems: true,
   },

   _create: function() {
      var self = this;

      this.palette = $('<div>').addClass('itemPalette');

      this.palette.on( 'click', '.item', function() {
         var itemSelected = $(this);

         if (!itemSelected.hasClass('disabled')) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
            self.palette.find('.item').removeClass('selected');
            itemSelected.addClass('selected');  
         }

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
      this.palette.find('.category').attr('update', 0);

      // grab the items in this category and sort them
      var arr = radiant.map_to_array(itemMap);
      
      arr.sort(function(a, b){
         var aName = a.display_name;
          var bName = b.display_name;
          if (aName > bName) {
            return 1;
          }
          if (aName < bName) {
            return -1;
          }
          // a must be equal to b
          return 0;
      });

      radiant.each(arr, function(i, item) {
         if (self.options.filter(item)) {
            var categoryElement = self._findCategory(item);
            var itemElement = self._findElementForItem(item)
            
            if (!itemElement) {
               itemElement = self._addItemElement(item);
               categoryElement.append(itemElement);
            }
            self._updateItemElement(itemElement, item);

            itemElement.attr('updated', 1)
         }
      })

      // anything that is not marked as updated needs to be removed
      self.palette.find('[updated=0]').remove();
      self.palette.find('.item').tooltipster();
   }.observes('inventory'),

   _findCategory: function(item) {
      var selector = "[category='" + item.category + "']";      
      var match = this.palette.find(selector)[0];

      if (match) {
         return $(match);
      } else {

         // new title element for the category
         $('<h2>')
            .html(item.category)
            .appendTo(this.palette);

         // the category container element that items are inserted into
         return $('<div>')
            .addClass('downSection')
            .attr('category', item.category)
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
         if (this.options.removeZeroNumItems) {
            itemEl.remove();
         } else {
            itemEl.addClass('disabled');
         }
      } else {
         itemEl.find('.num').html(num);
         itemEl.removeClass('disabled');
      }
   },
});
