var GrowWallsTool;

(function () {
   GrowWallsTool = SimpleClass.extend({

      toolId: 'growWallsTool',
      wallMaterialClass: 'growWallMaterial',
      columnMaterialClass: 'growColumnMaterial',

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               var wall = self._wallMaterial.getSelectedBrush();
               var column = self._columnMaterial.getSelectedBrush();
               return App.stonehearthClient.growWalls(column, wall);
            });
      },

      _onMaterialChange: function(type, brush) {
         var blueprint = this.buildingDesigner.getBlueprint();
         var constructionData = this.buildingDesigner.getConstructionData();

         if (blueprint && constructionData && constructionData.type == type) {
            App.stonehearthClient.replaceStructure(blueprint, brush);
         }
         // Re/activate the tool with the new material.
         this.buildingDesigner.reactivateTool(this.buildTool);
      },

      addTabMarkup: function(root) {
         var self = this;
         var brushes = self.buildingDesigner.getBuildBrushes();

         var click = function() {
            // Re/activate the floor tool with the new material.
            self.buildingDesigner.activateTool(self.buildTool);      
         };

         var tab = $('<div>', { id:self.toolId, class: 'tabPage'} );
         root.append(tab);

         self._wallMaterial = new MaterialHelper(tab,
                                                 self.buildingDesigner,
                                                 'Wall',
                                                 self.wallMaterialClass,
                                                 brushes.wall,
                                                 null,
                                                 false,
                                                 click);

         self._columnMaterial = new MaterialHelper(tab,
                                                 self.buildingDesigner,
                                                 'Column',
                                                 self.columnMaterialClass,
                                                 brushes.column,
                                                 null,
                                                 false,
                                                 click);
      },

      restoreState: function(state) {
         this._wallMaterial.restoreState(state);
         this._columnMaterial.restoreState(state);
      }
   });
})();
