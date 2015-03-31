var MaterialHelper = SimpleClass.extend({
   init : function(tab, buildingDesigner, tabTitle, materialClass, materials, clickHandler) {
      var self = this;

      self._buildingDesigner = buildingDesigner;

      self._container = $('<div>')
      this._materialClass = materialClass;
      self._clickHandler = clickHandler;

      tab.append(self._container)

      self._addMaterialPalette(tabTitle, materialClass, materials);
   },

   _buildMaterialPalette : function(category, items, materialClassName) {
      var palette = $('<div>').addClass('brushPalette');

      // for each category
      $.each(items, function(i, material) {
         var brush = $('<div>')
                        .addClass('brush')
                        .attr('brush', material.brush)
                        .attr('category', category)
                        .css({ 'background-image' : 'url(' + material.portrait + ')' })
                        .attr('title', material.name)
                        .addClass(materialClassName)
                        .append('<div class=selectBox />'); // for showing the brush when it's selected

         palette.append(brush);
      });

      return palette;
   },

   _selectBrush: function(brush, skipSave) {
      var container = this._container;
      var selectedMaterial;

      if (brush) {
         selectedMaterial = $(container.find('.brush[brush="' + brush + '"]')[0]);
         // if the brush doesn't exist, someone changed the backing
         // store.  pick the default
         if (!selectedMaterial[0]) {
            brush = undefined;
         }
      }
      if (!brush) {
         selectedMaterial = $(container.find('.brush')[0]);
         brush = selectedMaterial.attr('brush');
      }
      var category = selectedMaterial.attr('category');

      this._state.brush = brush
      this._state.brushes[category] = brush
      if (!skipSave) {
         this._buildingDesigner.saveKey(this._materialClass, this._state);
      }

      container.find('.button').removeClass('selected');
      container.find('.button[category=' + category + ']').addClass('selected');

      container.find('.brush').removeClass('selected');           // remove the selected box from all brushes
      selectedMaterial.addClass('selected');                // select the selected brush

      container.find('.materialSubTab').hide();                   // hide all sub palettes
      selectedMaterial.closest('.materialSubTab').show();   // show the one with the selected brush
   },

   restoreState: function(state) {
      this._state = state[this._materialClass]
      if (!this._state) {
         this._state = {
            brushes: {}
         }         
      }
      this._selectBrush(this._state.brush, true);
   },

   getSelectedBrush : function() {
      if (!this._state.brush) {
         this._selectBrush();
      }
      return this._state.brush;
   },

   _addMaterialPalette: function(tabTitle, materialClass, materials) {
      var self = this;      
      var toolbar;

      // add the selector...
      if (materials.length > 1) {
         toolbar = $('<div>').attr('id', 'materialToolbar')
                                 .addClass('downSection');
         self._container.append(toolbar);
      }

      // add the tabs...
      $.each(materials, function(i, material) {
         var category = material.category.toLowerCase()

         if (toolbar) {
            toolbar.append($('<img>')
                              .addClass('button')
                              .addClass(category)
                              .attr('category', category)
                              .append('<div class=selectBox />')
                           );
         }

         var subtab = $('<div>', {id:material.category, class: 'materialSubTab'});
         subtab.html($('<h2>')
                  .text(material.category + ' ' + tabTitle))
                  .append($('<div>', {class:'downSection'})
                  .append(self._buildMaterialPalette(category, material.items, materialClass)));
         self._container.append(subtab);
      });

      self._container.find('.' + materialClass).click(function() {
         var brush = $(this).attr('brush');

         self._container.find('.' + materialClass).removeClass('selected');
         $(this).addClass('selected');

         self._selectBrush(brush);
         self._clickHandler(brush);
      });

      if (toolbar) {
         toolbar.on('click', '.button', function() {
            var category = $(this).attr('category');
            var brush = self._state.brushes[category];
            if (!brush) {
               brush = self._container.find('.brush[category=' + category + ']').attr('brush');
            }
            self._selectBrush(brush);
            self._clickHandler(brush);
         });
      }

      self._container.find("[title]").tooltipster();
   }
});
