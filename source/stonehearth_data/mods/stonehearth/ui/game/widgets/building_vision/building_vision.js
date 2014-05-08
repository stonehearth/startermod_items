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

      this.$('#cycleVisionMode').click(function() {
         if (currentMode == 'normal') {
            setCurrentMode('xray');
         } else if (currentMode == 'xray') {
            setCurrentMode('rpg');
         } else {
            setCurrentMode('normal');
         }
      });

      this.$('#showAllWalls').click(function() {
        setCurrentMode('normal');
      });

      this.$('#hideFacingWalls').click(function() {
        setCurrentMode('xray');
      });

      this.$('#hideAllWalls').click(function() {
        setCurrentMode('rpg');
      });

      // tooltips
      this.$('#showAllWalls').tooltipster({
         content: $('<div class=title>' + i18n.t('stonehearth:building_vision_show_all_walls') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:building_vision_show_all_walls_description') + '</div>' /*+ 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + $(this).attr('hotkey')  + '</span></div>'*/)
      });

      this.$('#hideFacingWalls').tooltipster({
         content: $('<div class=title>' + i18n.t('stonehearth:building_vision_hide_facing_walls') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:building_vision_hide_facing_walls_description') + '</div>' /*+ 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + $(this).attr('hotkey')  + '</span></div>'*/)
      });

      this.$('#hideAllWalls').tooltipster({
         content: $('<div class=title>' + i18n.t('stonehearth:building_vision_hide_all_walls') + '</div>' + 
                    '<div class=description>' + i18n.t('stonehearth:building_vision_hide_all_walls_description') + '</div>' /*+ 
                    '<div class=hotkey>' + $.t('hotkey') + ' <span class=key>' + $(this).attr('hotkey')  + '</span></div>'*/)
      });

   }
});
