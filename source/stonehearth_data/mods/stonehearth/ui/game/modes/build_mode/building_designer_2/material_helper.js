var MaterialHelper = {};

MaterialHelper.buildMaterialPalette = function(materials, materialClassName) {
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
};

MaterialHelper.addMaterialTab = function(root, tabId) {
   var tab = $('<div>', {id:tabId, class: 'tabPage'});
   root.append(tab);
   return tab; 
};

MaterialHelper.addMaterialPalette = function(el, tabTitle, materialClass, materials, clickHandler) {
   el.append($('<h2>')
      .text(tabTitle))
   .append($('<div>', {class:'downSection'})
      .append(MaterialHelper.buildMaterialPalette(materials, materialClass))
   );

   el.find('.' + materialClass).click(function() {
      el.find('.' + materialClass).removeClass('selected');
      $(this).addClass('selected');
      var material = $(this).attr('index');

      clickHandler($(this).attr('brush'), material);
   });
};
