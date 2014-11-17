App.StonehearthBuildingDesignerTools = App.StonehearthBuildingDesignerBaseTools.extend({
   templateName: 'buildingDesigner',

   init: function() {
      var self = this;
      this._super();
   },   

   didInsertElement: function() {
      var self = this;

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

      // track placeable items and hand them off to the place item picker (in the html)
      this.set('tracker', 'stonehearth:placeable_item_inventory_tracker')

      // Make sure we call super after adding all the tools!
      this._super();
   }
});
