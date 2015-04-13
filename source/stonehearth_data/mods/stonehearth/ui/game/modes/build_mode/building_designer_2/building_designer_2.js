App.StonehearthBuildingDesignerTools = App.StonehearthBuildingDesignerBaseTools.extend({
   templateName: 'buildingDesigner',

   buildBrushes: null,

   init: function() {
      var self = this;
      self._super();

   },   

   didInsertElement: function() {
      var self = this;
      this._super();

      self._initializePalettes();
   },

   initializeTools : function() {
      if (!this._buildBrushes) {
         return;
      }
      if (!$.isEmptyObject(this.tools)) {
         return;
      }

      // TODO, maybe?  I think the argument can be made that all of these should just be sub-views,
      // and could be just declared directly within the HTML of whatever designer you want.  The
      // only wrinkle is that these views would need to expose the 'tool' interface as well, and
      // the base designer view would need to interrogate all child views to figure out which
      // is a tool and which is not.  Not hard, but kind of ugly.  At this point, I think it's more
      // 'cute' factoring than real design advantage, and so is left as an exercise to the bored.
      this.newTool(new PlaceFloorDecoTool({ category: 'decoration', toolId: 'placeDecorationTool'}));
      this.newTool(new PlaceFloorDecoTool({ category: 'furniture',  toolId: 'placeFurnitureTool'}));
      this.newTool(new PlaceFixtureTool({ category: 'doors',   toolId: 'drawDoorTool',   materialClass: 'doorMaterials'}));
      this.newTool(new PlaceFixtureTool({ category: 'windows', toolId: 'drawWindowTool', materialClass: 'windowMaterials'}));
      this.newTool(new DrawFloorTool({ toolId: 'drawFloorTool', sinkFloor:true }));
      this.newTool(new DrawFloorTool({ toolId: 'growFloorTool', sinkFloor:false }));
      this.newTool(new DrawWallTool);
      this.newTool(new GrowWallsTool);
      this.newTool(new GrowRoofTool);
      this.newTool(new DrawRoadTool);
      // this.newTool(new DrawSlabTool);

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
      this.$('#toolPaletteBuild').click();     
   },

   showEditor: function() {
      this._super();
      this.$('[tool=drawFloorTool]').click();
   },
});
