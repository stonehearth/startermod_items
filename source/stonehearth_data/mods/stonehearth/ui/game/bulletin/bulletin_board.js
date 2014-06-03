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

         self._lastViewedBulletinId = -1;
         self._bulletins = [];

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
                  var bulletinList = self._mapToList(bulletinBoard.bulletins);

                  // bulletin ids are monotonically increasing
                  bulletinList.sort(function (a, b) {
                     return a.id - b.id;
                  });

                  self._bulletins = bulletinList;
                  self._tryShowNextBulletin();
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

      _tryShowNextBulletin: function() {
         var self = this;

         if (self._bulletinNotificationView || self._bulletinDialogView) {
            return;
         }

         var bulletins = self._bulletins;
         var numBulletins = bulletins.length;

         for (var i = 0; i < numBulletins; i++) {
            var bulletin = bulletins[i]; 
            if (bulletin.id > self._lastViewedBulletinId) {
               self.showNotificationView(bulletin);
               return;
            }
         }
      },

      // should we be using an ember controller for these methods?
      showNotificationView: function(bulletin) {
         var self = this;
         self._bulletinNotificationView = App.gameView.addView(App.StonehearthBulletinNotification, { context: bulletin });
         self._lastViewedBulletinId = bulletin.id;
      },

      showDialogView: function(bulletin) {
         var self = this;
         var detailViewName = bulletin.config.ui_view;
         self._bulletinDialogView = App.gameView.addView(App[detailViewName], { context: bulletin })
      },

      onNotificationViewDestroyed: function () {
         var self = this;
         self._bulletinNotificationView = null;
         self._tryShowNextBulletin();
      },

      onDialogViewDestroyed: function () {
         var self = this;
         self._bulletinDialogView = null;
         self._tryShowNextBulletin();
      },

      getTrace: function() {
         return this._bulletinBoardTrace;
      }
   });
})();

