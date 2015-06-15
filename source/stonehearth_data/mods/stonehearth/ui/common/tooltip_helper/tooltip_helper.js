/***
 * A manager for handling portrait requests for entities.
 ***/
var StonehearthTooltipHelper;

(function () {
   StonehearthTooltipHelper = SimpleClass.extend({

      init: function() {
         var self = this;
         this._attributeTooltips = {};
         this._scoreTooltips = {};

         $.getJSON('/stonehearth/ui/data/tooltips.json', function(data) {
            radiant.each(data.attribute_tooltips, function(i, attributeData) {
               self._attributeTooltips[attributeData.attribute] = attributeData;
            })

            radiant.each(data.score_tooltips, function(i, scoreData) {
               self._scoreTooltips[scoreData.score] = scoreData;
            })
         });
      },

      getAttributeTooltip: function(attributeName) {
         if (attributeName in this._attributeTooltips) {
            var attributeData = this._attributeTooltips[attributeName];
            var tooltipString = '<div class="detailedTooltip"> <h2>' + attributeData.display_name
                                 + '</h2>'+ attributeData.description + '</div>';
            return tooltipString;
         }
         return null;
      },

      getAttributeData: function(attributeName) {
         return this._attributeTooltips[attributeName];
      },

      getScoreTooltip: function(scoreName) {
         if (scoreName in this._scoreTooltips) {
            var scoreData = this._scoreTooltips[scoreName];
            var tooltipString = '<div class="detailedTooltip"> <h2>' + scoreData.display_name
                                 + '</h2>'+ scoreData.description + '</div>';
            return tooltipString;
         }
         return null;
      },
   });
})();
