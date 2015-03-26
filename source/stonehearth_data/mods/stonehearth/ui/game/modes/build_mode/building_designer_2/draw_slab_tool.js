var DrawSlabTool;

(function () {
   DrawSlabTool = SimpleClass.extend({

      toolId: 'drawSlabTool',
      materialClass: 'slabMaterial',
      materialTabId: 'slabMaterialTab',
      brush: null,

      handlesType: function(type) {
         return type == 'slab';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.buildSlab(self.brush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Slab', self.materialClass, self.buildingParts.slabPatterns, 
                  function(brush) {
                     self.brush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('slabBrush', brush);

                     // Re/activate the slab tool with the new material.
                     self.buildingDesigner.reactivateTool(self.buildTool);
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
         var self = this;

         var selector = state.slabBrush ? '.' + self.materialClass + '[brush="' + state.slabBrush + '"]' :  '.' + self.materialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.materialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.brush = selectedMaterial.attr('brush');
      }
   });
})();
