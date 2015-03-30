var PlaceFloorDecoTool;

(function () {
   PlaceFloorDecoTool = SimpleClass.extend({

      toolId:   undefined,
      category: undefined,
      materialTabId: 'floorDecoMaterialTab',
      brush: null,

      init: function(o) {
         this.toolId = o.toolId;
         this.category = o.category;
         this.saveKey = this.toolId + 'Brush';
         this.materialTabId = this.toolId + 'MaterialTab';
      },

      inDom: function(buildingDesigner) {
         var self = this;

         this.buildingDesigner = buildingDesigner;
         this.buildTool = buildingDesigner.addTool(this.toolId)
            //.activate() -- anything special to do just before the tool is invoked?  Default hilights the tool button.
            //.fail() -- anything special to do on failure?  Default deactivates the tool.
            //.repeatOnSuccess(true/false) -- defaults to true.
            .invoke(function() {
               if (!self.brush) {
                  return undefined;
               }

               return function() {
                  var tip = App.stonehearthClient.showTip('stonehearth:item_placement_title', 'stonehearth:item_placement_description', { i18n: true });
                  return radiant.call('stonehearth:choose_place_item_location', self.brush)
                     .done(function(response) {
                        radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
                        App.stonehearthClient.hideTip();
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
         var tab = $('<div>', { id:self.toolId, class: 'tabPage'} );
         root.append(tab);
         var items = tab.append($('<div>').attr('id', 'items'));

         this._palette = items.stonehearthItemPalette({
            click: function(item) {
               var brush = item.attr('uri');

               // Remember what we've selected.
               self.buildingDesigner.saveKey(self.saveKey, brush);

               // Re/activate the tool with the new material.
               self.buildingDesigner.activateTool(self.buildTool);
            },

            filter: function(item) {
               return item.category == self.category;
            },
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

         var selector = state[self.saveKey] ? '.item[uri="' + state[self.saveKey] + '"]' :  '.item';
         var selectedMaterial = $(self._palette.find(selector)[0]);

         self._palette.find('.item').removeClass('selected');
         selectedMaterial.addClass('selected');
         self.brush = selectedMaterial.attr('uri');
      }
   });
})();
