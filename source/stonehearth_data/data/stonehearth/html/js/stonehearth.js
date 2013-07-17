(function () {
   var Stonehearth = SimpleClass.extend({

      _entityData: {},

      init: function() { 
         
      },

      entityData: function(entity, key, value) {
         this._entityData[entity] = this._entityData[entity] || {};
         
         if (arguments.length > 2) {
            this._entityData[entity][key] = value;
         }

         return this._entityData[entity][key];
      }
   });

   stonehearth = new Stonehearth();
})();
