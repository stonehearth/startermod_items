App.StonehearthBuildingDesignerTools = App.StonehearthBuildingDesignerBaseTools.extend({
   templateName: 'buildingDesigner',

   init: function() {
      var self = this;
      this._super();
   },   

   didInsertElement: function() {
      var self = this;

      self._initializePalettes();

      // TODO, maybe?  I think the argument can be made that all of these should just be sub-views,
      // and could be just declared directly within the HTML of whatever designer you want.  The
      // only wrinkle is that these views would need to expose the 'tool' interface as well, and
      // the base designer view would need to interrogate all child views to figure out which
      // is a tool and which is not.  Not hard, but kind of ugly.  At this point, I think it's more
      // 'cute' factoring than real design advantage, and so is left as an exercise to the bored.
      this.newTool(DrawFloorTool);
      this.newTool(DrawWallTool);
      this.newTool(GrowWallsTool);
      this.newTool(DrawDoodadTool);
      this.newTool(GrowRoofTool);
      this.newTool(DrawSlabTool);

      // Make sure we call super after adding all the tools!
      this._super();
   },

   // setup all the different main palettes at the top, like the little house graphic
   _initializePalettes: function() {
      var self = this;

      // palette selection
      this.$('[palette]').click(function(){
         var palette = $(this).attr('palette');

         self.$('.palette').hide();
         self.$('#' + palette).show();
      });

      // image map click handler
      this.$('#buildPaletteMap area').click(function() {
         var el = $(this);
         var tool = el.attr('tool');
         var palette = el.attr('palette');

         self.$('#' + palette + ' .selectionDisplay').css({
               'background-image' : 'url(/stonehearth/ui/game/modes/build_mode/building_designer_2/images/palettes/' + tool + '.png)'
            });
      });

      this.$('#toolPaletteBuild').click();
   }
});
