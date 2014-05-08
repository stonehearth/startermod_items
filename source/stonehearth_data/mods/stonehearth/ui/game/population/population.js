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

         var r  = new RadiantTrace()
         var trace = r.traceUri(this._populationUri, this.components);
         trace.progress(function(eobj) {
               self._population = eobj
            });
      },

      getData: function() {
         return this._population;
      },
   });
})();

