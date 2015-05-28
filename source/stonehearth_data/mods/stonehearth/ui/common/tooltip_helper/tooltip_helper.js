/***
 * A manager for handling portrait requests for entities.
 ***/
var StonehearthTooltipHelper;

(function () {
   StonehearthTooltipHelper = SimpleClass.extend({

      init: function() {
         var self = this;
         this._attributeTooltips = {};

         $.getJSON('/stonehearth/ui/data/tooltips.json', function(data) {
            radiant.each(data.attribute_tooltips, function(i, attributeData) {
               self._attributeTooltips[attributeData.attribute] = attributeData;
            })
         });
      },

      getAttributeTooltip: function(attributeName) {
         if (attributeName in this._attributeTooltips) {
            var attributeData = this._attributeTooltips[attributeName];
            var tooltipString = '<div class="attributeTooltip"> <h2>' + attributeData.display_name
                                 + '</h2>'+ attributeData.description + '</div>';
            return tooltipString;
         }
         return null;
      },
   });
})();
