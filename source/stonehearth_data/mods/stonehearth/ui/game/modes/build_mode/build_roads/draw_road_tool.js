var DrawRoadTool;

(function () {
   DrawRoadTool = SimpleClass.extend({

      toolId: 'drawRoadTool',
      materialClass: 'roadMaterial',
      materialTabId: 'roadMaterialTab',
      brush: null,

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
               return App.stonehearthClient.buildRoad(self.brush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Road Material', self.materialClass, self.buildingParts.floorPatterns, 
                  function(brush, material) {
                     self.brush = brush;
                     self.roadMaterial = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('roadMaterial', self.roadMaterial);

                     // Re/activate the road tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw road'})
         );
      },

      restoreState: function(state) {
         this.roadMaterial = state.roadMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.materialClass)[this.roadMaterial]);
         selectedMaterial.addClass('selected');
         this.brush = selectedMaterial.attr('brush');
      }
   });
})();
