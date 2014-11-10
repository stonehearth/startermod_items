var DrawSlabTool;

(function () {
   DrawSlabTool = SimpleClass.extend({

      toolId: 'drawSlabTool',
      materialClass: 'slabMaterial',
      materialTabId: 'slabMaterialTab',
      brush: null,

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.buildSlab(self.brush);
            })
            .done();
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Slab Material', self.materialClass, self.buildingParts.slabPatterns, 
                  function(brush, material) {
                     self.brush = brush;
                     self.material = material;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('slabMaterial', self.floorMaterial);

                     // Re/activate the floor tool with the new material.
                     App.stonehearthClient.callTool(self.buildTool);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw slab'})
         );
      },

      restoreState: function(state) {
         this.material = state.slabMaterial || 0;
         var selectedMaterial = $($('#' + this.materialTabId + ' .' + this.materialClass)[this.material]);
         selectedMaterial.addClass('selected');
         this.brush = selectedMaterial.attr('brush');
      }
   });
})();
