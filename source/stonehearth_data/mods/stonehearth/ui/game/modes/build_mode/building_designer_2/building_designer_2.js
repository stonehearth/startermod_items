App.StonehearthBuildingDesignerTools = App.StonehearthBuildingDesignerBaseTools.extend({
   templateName: 'buildingDesigner',
   init: function() {
      var self = this;

      this._super();
   },   

   didInsertElement: function() {
      var self = this;

      this.newTool(DrawFloorTool);
      this.newTool(DrawWallTool);
      this.newTool(GrowWallsTool);
      this.newTool(DrawDoodadTool);
      this.newTool(GrowRoofTool);
      this.newTool(DrawSlabTool);

      this._super();

      // undo/redoo tool
      this.$('#undoTool').click(function() {
         if (self.get('building')) {
            App.stonehearthClient.undo();
         }
      });

      var doEraseStructure = function() {
         App.stonehearthClient.eraseStructure(
            self.activateElement('#eraseStructureTool'))
            .fail(self._deactivateTool('#eraseStructureTool'))
            .done(function() {
               doEraseStructure();
            });
      };

      this.$('#eraseStructureTool').click(function() {
         doEraseStructure();
      });
   }
});

