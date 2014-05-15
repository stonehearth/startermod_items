var StonehearthPopulation;

(function () {
   StonehearthPopulation = SimpleClass.extend({

      components: {
         "citizens" : {
            "*" : {
               "unit_info": {},           
               "stonehearth:ai" : {},
               "stonehearth:profession" : {
                  "profession_uri" : {}
               },
               "stonehearth:crafter" : {
                  "workshop" : {}
               },
            }
         }
      },

      init: function() {
         var self = this;

         radiant.call('stonehearth:get_population')
            .done(function(response){
               self._populationUri = response.population;
               self._createTrace();
            });
      },

      _createTrace: function() {
         var self = this;

         this._radiantTrace = new RadiantTrace();
         this._populationTrace = this._radiantTrace.traceUri(this._populationUri, this.components);
      },

      getTrace: function() {
         return this._populationTrace;
      },
   });
})();

