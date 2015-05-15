var DrawRoadTool;

(function () {
   DrawRoadTool = SimpleClass.extend({

      toolId: 'drawRoadTool',
      roadMaterialClass: 'roadMaterial',

      handlesType: function(type) {
         return type == 'road';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               var road = self._roadMaterial.getSelectedBrush();
               if (road == 'eraser') {
                  return App.stonehearthClient.eraseStructure();
               }
               return App.stonehearthClient.buildRoad(road);
            });
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


         self._roadMaterial = new MaterialHelper(tab,
                                                 self.buildingDesigner,
                                                 'Road',
                                                 self.materialClass,
                                                 brushes.voxel,
                                                 brushes.pattern,
                                                 true,
                                                 click);
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw road'})
         );
      },

      restoreState: function(state) {
         this._roadMaterial.restoreState(state);
      }
   });
})();
