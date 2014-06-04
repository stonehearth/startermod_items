var StonehearthBulletinBoard;

(function () {
   StonehearthBulletinBoard = SimpleClass.extend({

      components: {
         "bulletins" : {
            "*" : {
               "config" : {} // unused
            }
         }
      },

      init: function() {
         var self = this;

         self._lastViewedBulletinId = -1;

         radiant.call('stonehearth:get_bulletin_board_datastore')
            .done(function(response) {
               self._bulletinBoardUri = response.bulletin_board;
               self._createTrace();
            });

         self._orderedBulletins = Ember.A();
      },

      _createTrace: function() {
         var self = this;

         self._radiantTrace = new RadiantTrace();
         self._bulletinBoardTrace = self._radiantTrace.traceUri(self._bulletinBoardUri, self.components)
            .progress(function(bulletinBoard) {
               if (bulletinBoard.bulletins) {
                  var list = self._mapToList(bulletinBoard.bulletins);

                  list.sort(function (a, b) {
                     return a.id - b.id;
                  });

                  self._orderedBulletins.clear();
                  self._orderedBulletins.pushObjects(list);

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

      // unused
      // used to remove flicker when updating the bulletin list
      // side effect is that the old elements in the list will not update,
      //    since the ListView is not (yet) tracking them individually
      _updateOrderedBulletins: function(updatedList) {
         var self = this;
         var orderedBulletins = self._orderedBulletins;
         var oldLength = orderedBulletins.length;
         var newLength = updatedList.length;
         var shortestLength = Math.min(oldLength, newLength);
         var i, j;

         // find the first index that differs
         for (i = 0; i < shortestLength; i++) {
            if (orderedBulletins[i].id != updatedList[i].id) {
               break;
            }
         }

         // delete everything after that index
         for (j = i; j < oldLength; j++) {
            orderedBulletins.popObject();
         }

         // copy the remaining items from the updatedList
         for (j = i; j < newLength; j++) {
            orderedBulletins.pushObject(updatedList[j]);
         }

         for (i = 0; i < newLength; i++) {
            if (orderedBulletins[i] != updatedList[i]) {
               console.log('Error: orderedBulletins is not correct!')
               break;
            }
         }
      },

      _tryShowNextBulletin: function() {
         var self = this;

         if (self._bulletinNotificationView || self._bulletinDialogView) {
            return;
         }

         var bulletins = self._orderedBulletins;
         var numBulletins = bulletins.length;

         for (var i = 0; i < numBulletins; i++) {
            var bulletin = bulletins[i]; 
            if (bulletin.id > self._lastViewedBulletinId) {
               self.showNotificationView(bulletin);
               return;
            }
         }
      },

      showNotificationView: function(bulletin) {
         var self = this;
         self._bulletinNotificationView = App.gameView.addView(App.StonehearthBulletinNotification, { uri: bulletin.__self });
         self._lastViewedBulletinId = bulletin.id;
      },

      showDialogView: function(bulletin) {
         var self = this;
         var dialogViewName = bulletin.ui_view;
         if (dialogViewName) {
            self._bulletinDialogView = App.gameView.addView(App[dialogViewName], { uri: bulletin.__self });
         } else {
            self.onDialogViewDestroyed(bulletin);
         }
         // TODO: dismiss bullitinListView if bullitins list will become empty
      },

      toggleListView: function() {
         var self = this;

         // toggle the view
         if (!self._bulletinListView || self._bulletinListView.isDestroyed) {
            self._bulletinListView = App.gameView.addView(App.StonehearthBulletinList, { context: self._orderedBulletins });
         } else {
            self._bulletinListView.destroy();
            self._bulletinListView = null;
         }
      },

      zoomToLocation: function(bulletin) {
         var entity = bulletin.data.zoom_to_entity;
         
         if (entity) {
            radiant.call('stonehearth:camera_look_at_entity', entity);
         }
      },

      onNotificationViewDestroyed: function(bulletin) {
         var self = this;
         self._bulletinNotificationView = null;
         self._tryShowNextBulletin();
      },

      onDialogViewDestroyed: function(bulletin) {
         var self = this;
         self._bulletinDialogView = null;
         radiant.call('stonehearth:remove_bulletin', bulletin.id);
         self._tryShowNextBulletin();
      },

      getTrace: function() {
         return this._bulletinBoardTrace;
      }
   });
})();
