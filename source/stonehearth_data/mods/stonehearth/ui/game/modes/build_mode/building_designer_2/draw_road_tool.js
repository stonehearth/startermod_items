var DrawRoadTool;

(function () {
   DrawRoadTool = SimpleClass.extend({

      toolId: 'drawRoadTool',
      roadMaterialClass: 'roadMaterial',
      curbMaterialClass: 'curbMaterial',

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
               var curb = self._curbMaterial.getSelectedBrush();
               var road = self._roadMaterial.getSelectedBrush();
               return App.stonehearthClient.buildRoad(road, curb == 'none' ? null : curb);
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


               self._roadMaterial = new MaterialHelper(tab,
                                                       self.buildingDesigner,
                                                       'Road',
                                                       self.roadMaterialClass,
                                                       self.buildingParts.roadPatterns,
                                                       click);

               self._curbMaterial = new MaterialHelper(tab,
                                                       self.buildingDesigner,
                                                       'Curb',
                                                       self.curbMaterialClass,
                                                       self.buildingParts.curbPatterns,
                                                       click);
         });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Draw road'})
         );
      },

      restoreState: function(state) {
         this._curbMaterial.restoreState(state);
         this._roadMaterial.restoreState(state);
      }
   });
})();
