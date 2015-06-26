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

      getAttributeTooltip: function(attributeName, additionalTooltipData) {
         if (this.hasAttributeTooltip(attributeName)) {
            var attributeData = this._attributeTooltips[attributeName];
            var tooltipString = '<div class="detailedTooltip"> <h2>' + attributeData.display_name
                                 + '</h2>'+ attributeData.description;
            if (attributeData.bullet_points) {
               tooltipString = tooltipString + "<ul>";

               radiant.each(attributeData.bullet_points, function(i, bullet) {
                  tooltipString = tooltipString + "<li>" + bullet + "</li>";
               });

               tooltipString = tooltipString + "</ul>";
            }

            if (additionalTooltipData) {
               tooltipString = tooltipString + additionalTooltipData;
            }

            tooltipString = tooltipString + '</div>';

            return tooltipString;
         }
         return null;
      },

      hasAttributeTooltip: function(attributeName) {
         return attributeName in this._attributeTooltips;
      },

      getScoreTooltip: function(scoreName, isTownDescription, additionalTooltipData) {
         if (this.hasScoreTooltip(scoreName)) {
            var scoreData = this._scoreTooltips[scoreName];
            var description = isTownDescription? scoreData.town_description : scoreData.description;
            var tooltipString = '<div class="detailedTooltip"> <h2>' + scoreData.display_name
                                 + '</h2>'+ description

            if (scoreData.bullet_points) {
               tooltipString = tooltipString + "<ul>";

               radiant.each(scoreData.bullet_points, function(i, bullet) {
                  tooltipString = tooltipString + "<li>" + bullet + "</li>";
               });

               tooltipString = tooltipString + "</ul>";
            }

            if (additionalTooltipData) {
               tooltipString = tooltipString + additionalTooltipData;
            }

            tooltipString = tooltipString + '</div>';

            return tooltipString;
         }
         return null;
      },

      hasScoreTooltip: function(scoreName) {
         return scoreName in this._scoreTooltips;
      },
   });
})();
