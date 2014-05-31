var StonehearthBulletinBoard;

(function () {
   StonehearthBulletinBoard = SimpleClass.extend({

      components: {
         "bulletins" : []
      },

      init: function() {
         var self = this;

         radiant.call('stonehearth:get_bulletin_board_datastore')
            .done(function(response) {
               self._bulletinBoardUri = response.bulletin_board;
               self._createTrace();
            });
      },

      _createTrace: function() {
         var self = this;

         this._radiantTrace = new RadiantTrace();
         this._bulletinBoardTrace = this._radiantTrace.traceUri(this._bulletinBoardUri, this.components)
            .progress(function(bulletinBoard) {
               // ok for length to be 0, but it must exist (i.e. it has to be serialized as an array)
               if (bulletinBoard.bulletins && bulletinBoard.bulletins.length) {
                  // TODO: replace this with a function that manages the queue of bulletins
                  // Note that the id of the bulletins are monotonically increasing
                  var num_bulletins = bulletinBoard.bulletins.length;
                  if (num_bulletins > 0) {
                     var bulletin = bulletinBoard.bulletins[num_bulletins-1];
                     $(top).trigger("bulletin_board_changed", bulletin.__self);

                     //App.stonehearth.bulletinBoard.lastBulletinId = bulletin.id;
                  }
               }
            });
      },

      getTrace: function() {
         return this._bulletinBoardTrace;
      }
   });
})();

