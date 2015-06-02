$.widget( "stonehearth.stonehearthRoster", {
   _create: function() {
      var self = this;

      this.palette = $('<div>').addClass('roster');

      this.palette.on( 'click', '.item', function() {
         // If we want some on click functionality
         return false;
      });

      this.element.append(this.palette);
   },

   updateRoster: function(rosterArray) {
      var self = this;

      // mark all items as not updated
      this.palette.find('.rosterEntry').attr('updated', 0);   

      radiant.each(rosterArray, function(i, entry) {
         var rosterElement = self._findElementForEntry(entry)
         
         if (!rosterElement) {
            rosterElement = self._addRosterElement(entry);
            self.palette.append(rosterElement);
         } else {
            self._updateRosterElement(rosterElement, entry);
         }

         rosterElement.attr('updated', 1)
      })

      // anything that is not marked as updated needs to be removed
      self.palette.find('[updated=0]').remove();
   },

   _findElementForEntry: function(entry) {
      var selector = "[__id='" + entry.__id + "']";      
      var match = this.palette.find(selector)[0];

      if (match) {
         return $(match);
      } else {
         return null;
      }
   },

   _addRosterElement: function(entry) {
      var img = $('<img>')
         .addClass('portrait')
         .attr('src', entry.portrait);

      var name = $('<input>')
         .addClass('name')
         .attr('onClick', 'this.select();')
         .attr('type', 'text')
         .attr('value', entry.unit_info.name);

      name.keypress(function (e) {
            if (e.which == 13) {
               radiant.call('stonehearth:set_display_name', entry.__self, $(this).val())
               $(this).blur();
           }
         });

      // TODO(yshan): add traits.
      var traits = $('<div>')
         .addClass('traits')
         .html("Normal");

      var stats = $('<div>')
         .addClass('stats');

      var attributesToDisplay = ['mind', 'body', 'spirit'];
      radiant.each(attributesToDisplay, function(i, attribute) {
         var attributeValue = entry['stonehearth:attributes'].attributes[attribute].effective_value;
         var element = $('<div>')
            .addClass(attribute)
            .html(attributeValue);
         
         element.tooltipster();            
         var tooltipString = App.tooltipHelper.getAttributeTooltip(attribute);
         element.tooltipster('content', $(tooltipString));
         
         stats.append(element);
      });

      var rosterElement = $('<div>')
         .addClass('rosterEntry')
         .attr('__id', entry.__id)
         .append(img)
         .append(name)
         .append(traits)
         .append(stats);

      return rosterElement;
   },

   _updateRosterElement: function(elem, entry) {
      elem.find('.portrait').attr('src', entry.portrait);
      elem.find('.name').attr('value', entry.unit_info.name);
   },
});
