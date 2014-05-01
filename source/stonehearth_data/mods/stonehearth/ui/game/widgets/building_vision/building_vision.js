App.StonehearthBuildingVisionWidget = App.View.extend({
   templateName: 'stoneheartBuildingVision',

   didInsertElement: function() {
      var self = this;
      var palette = this.$('#palette');

      // input handlers
      this.$().hover(
         function() {
            palette.fadeIn();      
         },
         function() {
            palette.fadeOut();
         });

      this.$('#showAllWalls').click(function() {

      });

      this.$('#hideFacingWalls').click(function() {

      });

      this.$('#hideAllWalls').click(function() {

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
