var DrawWallTool;

(function () {
   DrawWallTool = SimpleClass.extend({

      toolId: 'drawWallTool',
      wallMaterialClass: 'wallMaterial',
      columnMaterialClass: 'columnMaterial',

      handlesType: function(type) {
         return type == 'wall' || type == 'column';
      },


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
               return App.stonehearthClient.buildWall(column, wall);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

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
                                                       self.buildingParts.wallPatterns,
                                                       click);

               self._columnMaterial = new MaterialHelper(tab,
                                                       self.buildingDesigner,
                                                       'Column',
                                                       self.columnMaterialClass,
                                                       self.buildingParts.columnPatterns,
                                                       click);
         });
      },

      activateOnBuilding: function() {
         var blueprint = this.buildingDesigner.getBlueprint();
         var constructionData = this.buildingDesigner.getConstructionData();

         if (blueprint && constructionData) {
            if (constructionData.type == 'column') {

            } else if (constructionData.type == 'wall') {
               
            }
         } 
      },

      restoreState: function(state) {
         this._wallMaterial.restoreState(state);
         this._columnMaterial.restoreState(state);
      }
   });
})();
