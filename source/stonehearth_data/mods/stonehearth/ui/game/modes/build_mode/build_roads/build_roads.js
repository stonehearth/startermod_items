App.StonehearthBuildRoadsView = App.View.extend({
   templateName: 'stonehearthBuildRoads',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', "gui"],

   _buildMaterialPalette: function(materials, materialClassName) {
      var palette = $('<div>').addClass('brushPalette');

      // for each category
      var index = 0;
      $.each(materials, function(i, category) {
         // for each material
         $.each(category.items, function(k, material) {
            var brush = $('<div>')
                           .addClass('brush')
                           .attr('brush', material.brush)
                           .css({ 'background-image' : 'url(' + material.portrait + ')' })
                           .attr('title', material.name)
                           .attr('index', index)
                           .addClass(materialClassName)
                           .append('<div class=selectBox />'); // for showing the brush when it's selected

            palette.append(brush);
            

            index += 1;
         });
      });

      return palette;
   },

   _deactivateTool: function(toolTag) {
      var self = this;
      return function() {
         if (self.$(toolTag)) {
            self.$(toolTag).removeClass('active');
         }
         $(top).trigger('radiant_hide_tip');
      }
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      var activateElement = function(elPath) {
         return function() {
            if (self.$(elPath)) {
               self.$(elPath).addClass('active');   
            }
         }
      };

      // draw floor tool
      var doDrawRoad = function() {
         var brush = self.$('#roadMaterialTab .roadMaterial.selected').attr('brush');
         App.stonehearthClient.buildRoad(brush, activateElement('#drawRoadTool'))
            .fail(self._deactivateTool('#drawRoadTool'))
            .done(function() {
               doDrawRoad();
            });
      };

      var doEraseRoad = function() {
         App.stonehearthClient.eraseFloor(activateElement('#eraseFloorTool'))
            .fail(self._deactivateTool('#eraseFloorTool'))
            .done(function() {
               doEraseFloor(false);
            });
      };



      // tab buttons and pages
      this.$('.toolButton').click(function() {
         var tool = $(this);
         var tabId = tool.attr('tab');
         var tab = self.$('#' + tabId);

         if (!tabId) {
            return;
         }

         // show the correct tab page
         self.$('.tabPage').hide();
         tab.show();

         // activate the tool
         self.$('.toolButton').removeClass('active');
         tool.addClass('active');
         
         // update the material in the tab to reflect the selection
         //self._selectActiveMaterial(tab);
      });

      this.$('#showOverview').click(function() {
         self.showOverview();
      });

      this.$('#showEditor').click(function() {
         self.showEditor();
      });

      this.$('#drawRoadTool').click(function() {
         //doDrawRoad();
      });

      this.$('#eraseRoadTool').click(function() {
         doEraseRoad(true);
      });

      $.get('/stonehearth/data/build/building_parts.json')
         .done(function(json) {
            self.buildingParts = json;
            self.$('#roadMaterials').append(self._buildMaterialPalette(self.buildingParts.floorPatterns, 'roadMaterial'));
            //$(self.$('#roadMaterialTab .roadMaterial')[self._state.roadMaterial]).addClass('selected');

            /*self._addEventHandlers();
            self._restoreUiState();
            self._updateControls();*/

            self.$('.roadMaterial').click(function() {
               self.$('.roadMaterial').removeClass('selected');
               $(this).addClass('selected');

               //self._state.roadMaterial = $(this).attr('index');
               //self._saveState();

               // Re/activate the floor tool with the new material
               doDrawRoad(true);
            });
         });
   },

   showOverview: function() {
      this.$('#editor').hide();
      this.$('#overview').show();
   },

   showEditor: function() {
      this.$('#editor').show();
      this.$('#overview').hide();
   },

});

