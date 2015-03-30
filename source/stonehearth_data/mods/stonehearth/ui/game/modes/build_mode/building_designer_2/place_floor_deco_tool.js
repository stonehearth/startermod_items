var PlaceFloorDecoTool;

(function () {
   PlaceFloorDecoTool = SimpleClass.extend({

      toolId: 'placeFloorDecoTool',
      materialClass: 'floorDecoMaterials',
      materialTabId: 'floorDecoMaterialTab',
      brush: null,

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               return function() {
                  var tip = App.stonehearthClient.showTip('stonehearth:item_placement_title', 'stonehearth:item_placement_description', { i18n: true });
                  return radiant.call('stonehearth:choose_place_item_location', self.brush)
                     .done(function(response) {
                        radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
                        self.hideTip();
                     })
               };
            });
      },

      addTabMarkup: function(root) {
         var self = this;
         if (this._trace) {
            return;
         }

         // build the palette
         var tab = MaterialHelper.addMaterialTab(root, self.materialTabId);
         var items = tab.append($('<div>').attr('id', 'items'));

         this._palette = items.stonehearthItemPalette({
            click: function(item) {
               self.brush = item.attr('uri');

               // Remember what we've selected.
               self.buildingDesigner.saveKey('floorDecoBrush', self.brush);

               // Re/activate the floorDeco tool with the new material.
               self.buildingDesigner.activateTool(self.buildTool);
            }
         });

         radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', 'stonehearth:placeable_item_inventory_tracker')
            .done(function(response) {
               self._trace = new StonehearthDataTrace(response.tracker, {})
                  .progress(function(response) {
                     //var itemPaletteView = self._getClosestEmberView(self.$('.itemPalette'));
                     //itemPaletteView.updateItems(response.tracking_data);
                     self._palette.stonehearthItemPalette('updateItems', response.tracking_data);
                  });
            })
            .fail(function(response) {
               console.error(response);
            });
      },

      addButtonMarkup: function(root) {
         root.append(
            $('<div>', {id:this.toolId, class:'toolButton', tab:this.materialTabId, title:'Place FloorDecos'})
         );
      },

      restoreState: function(state) {
         var self = this;

         var selector = state.floorDecoBrush ? '.' + self.materialClass + '[brush="' + state.floorDecoBrush + '"]' :  '.' + self.materialClass;
         var selectedMaterial = $($(selector)[0]);
         $('.' + self.materialClass).removeClass('selected');
         selectedMaterial.addClass('selected');
         self.brush = selectedMaterial.attr('brush');
      }
   });
})();
