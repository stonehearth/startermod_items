var DrawDoodadTool;

(function () {
   DrawDoodadTool = SimpleClass.extend({

      toolId: 'drawDoodadTool',
      materialClass: 'doodadMaterials',
      materialTabId: 'doodadMaterialTab',
      brush: null,

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.addDoodad(self.brush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, '', self.materialClass, self.buildingParts.doodads, 
                  function(brush) {
                     self.brush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('doodadBrush', brush);

                     // Re/activate the doodad tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Place doors & windows'})
         );
      },

      restoreState: function(state) {
         var self = this;

         var selector = state.doodadBrush ? '.' + self.materialClass + '[brush="' + state.doodadBrush + '"]' :  '.' + self.materialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.materialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.brush = selectedMaterial.attr('brush');
      }
   });
})();
