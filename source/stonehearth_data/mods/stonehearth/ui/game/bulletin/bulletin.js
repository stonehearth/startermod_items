var StonehearthBulletin;

(function () {
   StonehearthBulletin = SimpleClass.extend({

      init: function() {
         var self = this;

         this._initFacade();
         
         /*
         radiant.call('stonehearth:get_population')
            .done(function(response){
               self._populationUri = response.population;
               self._createTrace();
            });
         */       
      },

      // totally fake source of bulletin data for testing
      _initFacade: function() {
         var self = this;
         
         this.bulletinData = [];

         // key handler to add bulletings
         $(document).keyup(function(e) {
            if(e.keyCode == 66) { /* b */
               // add a bulletin
               self._appendBulletin();
            }
         });
      },

      _appendBulletin: function() {
         var n = this.bulletinData.length;

         var newBulletin = {
            bulletin_info: {
               name: 'bulletin ' + n,
               type: 'info'
            },
            time_stamp: n,
            entity: 0
         }

         this.bulletinData.push(newBulletin);

         $(top).trigger('new_bulletin', {
               bulletins : this.bulletinData
            });
      },

      /*
      _createTrace: function() {
         this._radiantTrace = new RadiantTrace();
         this._populationTrace = this._radiantTrace.traceUri(this._populationUri, this.components);
      },

      getTrace: function() {
         return this._populationTrace;
      },

      */

   });
})();

