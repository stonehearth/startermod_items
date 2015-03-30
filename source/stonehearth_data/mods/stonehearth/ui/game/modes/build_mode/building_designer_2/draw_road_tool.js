var DrawRoadTool;

(function () {
   DrawRoadTool = SimpleClass.extend({

      toolId: 'drawRoadTool',
      roadMaterialClass: 'roadMaterial',
      curbMaterialClass: 'curbMaterial',
      materialTabId: 'roadMaterialTab',
      roadBrush: null,
      curbBrush: null,

      handlesType: function(type) {
         return type == 'road' || type == 'curb';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return App.stonehearthClient.buildRoad(self.roadBrush, self.curbBrush == 'none' ? null : self.curbBrush);
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         $.get('/stonehearth/data/build/building_parts.json')
            .done(function(json) {
               self.buildingParts = json;

               var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
               MaterialHelper.addMaterialPalette(tab, 'Road', self.roadMaterialClass, self.buildingParts.roadPatterns, 
                  function(brush) {
                     self.roadBrush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('roadBrush', brush);

                     // Re/activate the road tool with the new material.
                     self.buildingDesigner.activateTool(self.buildTool);
                  }
               );
               MaterialHelper.addMaterialPalette(tab, 'Curb', self.curbMaterialClass, self.buildingParts.curbPatterns, 
                  function(brush) {
                     self.curbBrush = brush;

                     // Remember what we've selected.
                     self.buildingDesigner.saveKey('curbBrush', brush);

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
         var self = this;

         var selector = state.roadBrush ? '.' + self.roadMaterialClass + '[brush="' + state.roadBrush + '"]' :  '.' + self.roadMaterialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.roadMaterialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.roadBrush = selectedMaterial.attr('brush');

         var selector = state.curbBrush ? '.' + self.curbMaterialClass + '[brush="' + state.curbBrush + '"]' :  '.' + self.curbMaterialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.curbMaterialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.curbBrush = selectedMaterial.attr('brush');
      }
   });
})();
