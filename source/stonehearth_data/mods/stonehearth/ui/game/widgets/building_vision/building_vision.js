App.StonehearthBuildingVisionWidget = App.View.extend({
   templateName: 'stonehearthBuildingVision',

   modeChangeClickHandler: function() {
      var self = this;
      return function() {
         var currentMode = App.getVisionMode();

         if (currentMode == 'normal') {
            currentMode = 'xray';
         } else if (currentMode == 'xray') {
            currentMode = 'rpg';
         } else {
            currentMode = 'normal';
         }
         App.setVisionMode(currentMode);
      };
   },

   modeChangeHandler: function() {
      var self = this;
      return function(e, newMode) {
         self.$('#visionButton').attr('class', newMode);
      };
   },

   didInsertElement: function() {
      this._super();

      this.$(top).on('stonehearthVisionModeChange', this.modeChangeHandler());

      this.$('#visionButton').click(this.modeChangeClickHandler());

      this.$('#visionButton').tooltipster({
         content: $('<div class=title>' + i18n.t('stonehearth:building_vision') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:building_vision_description') + '</div>' + 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + this.$('#visionButton').attr('hotkey')  + '</span></div>')
      });
   }
});
