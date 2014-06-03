var StonehearthBulletinBoard;

(function () {
   StonehearthBulletinBoard = SimpleClass.extend({

      components: {
         "bulletins" : {
            "*" : {
               "config" : {}
            }
         }
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

         self._radiantTrace = new RadiantTrace();
         self._bulletinBoardTrace = self._radiantTrace.traceUri(self._bulletinBoardUri, self.components)
            .progress(function(bulletinBoard) {
               if (bulletinBoard.bulletins) {
                  // TODO: replace this with a function that manages the queue of bulletins
                  // Note that the id of the bulletins are monotonically increasing
                  var bulletinList = self._mapToList(bulletinBoard.bulletins);
                  bulletinList.sort(function (a, b) {
                     return a.creation_time - b.creation_time;
                  });

                  var num_bulletins = bulletinList.length;
                  if (num_bulletins > 0) {
                     var bulletin = bulletinList[num_bulletins-1];
                     $(top).trigger("bulletin_board_changed", bulletin);
                     //App.stonehearth.bulletinBoard.lastBulletinId = bulletin.id;
                  }
               }
            });
      },

      _mapToList: function(map) {
         var list = [];
         $.each(map, function(key, value) {
            if (key != "__self" && map.hasOwnProperty(key)) {
               list.push(value);
            }
         });
         return list;
      },

      getTrace: function() {
         return this._bulletinBoardTrace;
      }
   });
})();

