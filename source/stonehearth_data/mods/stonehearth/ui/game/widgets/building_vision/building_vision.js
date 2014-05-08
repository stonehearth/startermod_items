App.StonehearthBuildingVisionWidget = App.View.extend({
   templateName: 'stoneheartBuildingVision',

   didInsertElement: function() {
      var self = this;
      var palette = this.$('#palette');
      var currentMode = 'normal';

      var setCurrentMode = function(mode) {
        currentMode = mode;
        radiant.call('stonehearth:set_building_vision_mode', currentMode);
      }

      // input handlers
      this.$().hover(
         function() {
            palette.fadeIn();      
         },
         function() {
            palette.fadeOut();
         });

      this.$('#visionButton').click(function() {
         self.$('#visionButton').removeClass(currentMode);

         if (currentMode == 'normal') {
            setCurrentMode('xray');
         } else if (currentMode == 'xray') {
            setCurrentMode('rpg');
         } else {
            setCurrentMode('normal');
         }

         self.$('#visionButton').addClass(currentMode);
      });


      this.$('#visionButton').tooltipster({
         content: $('<div class=title>' + i18n.t('stonehearth:building_vision') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:building_vision_description') + '</div>' + 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + self.$('#visionButton').attr('hotkey')  + '</span></div>')
      });

   }
});
