App.StonehearthBuildingCostView = App.View.extend({
   templateName: 'buildingCost',
   i18nNamespace: 'stonehearth',

   didInsertElement: function() {
      console.log(this.get('context'));
   },

   _transformCost: function() {
      var self = this;
      var costArr = [];

      var cost = self.get('cost')
      self._addResourcesToCost(costArr, cost.resources)
      self._addItemsToCost(costArr, cost.items)

      self.set('building_cost', costArr);

   }.observes('cost'),

   _addResourcesToCost: function(arr, map) {
      if (map) {
         $.each(map, function(material, count) {
            if(map.hasOwnProperty(material)) {
               var formatting = App.constants.formatting.resources[material];

               // rough approximation of the # entitites required 
               var entityCount = Math.max(1, parseInt(count / formatting.stacks)); 

               arr.push({
                  name: formatting.name,
                  icon: formatting.icon,
                  count: entityCount,
                  available: "*",
               });
            }
         });
      }
   },

   _addItemsToCost: function(arr, map) {
      if (map) {
         $.each(map, function(uri, item) {
            if(map.hasOwnProperty(uri)) {
               arr.push({
                  name: item.name,
                  icon: item.icon,
                  count: item.count,
                  available: "*",
               });
            }
         });
      }
   },   
   
});
