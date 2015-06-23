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

      init: function(initCompletedDeferred) {
         var self = this;
         this._initCompleteDeferred = initCompletedDeferred;

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
                  self._bulletins = bulletinBoard.bulletins
                  var list = radiant.map_to_array(bulletinBoard.bulletins);

                  list.sort(function (a, b) {
                     return a.id - b.id;
                  });

                  self._orderedBulletins.clear();
                  self._orderedBulletins.pushObjects(list);

                  self._destroyMissingView('_bulletinDialogView');
                  self._destroyMissingView('_bulletinNotificationView');
                  self._tryShowNextBulletin();
               }
            });

         this._initCompleteDeferred.resolve();
      },

      _destroyMissingView: function(viewName) {
         // Remove the view referenced by `viewName` if it is no longer in the model
         var self = this;        
         var view = self[viewName];
         if (!view) {
            return;
         }
         var id = view.get('model.id');
         if (!id) {
            return;
         }
         if (!self._bulletins[id]) {
            self[viewName] = null;
            view.destroy();
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
            //The bulletin keeps track of whether it's been shown, but this updates
            //too late to be in perfect sync with the UI. So keep track of what was
            //shown this session, but on load, use the shown to prevent all the shown bulletins
            //from appearing again. This seems...redundant, happy to take suggestions. 
            if (!bulletin.shown && (bulletin.id > self._lastViewedBulletinId)) {
               self.showNotificationView(bulletin);
               radiant.call('stonehearth:mark_bulletin_shown', bulletin.id);
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
         if (self._bulletinDialogView && self._bulletinDialogView.get('model').id == bulletin.id) {
            // Trying to display dialog that's already showing! Ignore the request.
            return;
         }

         if (self._bulletinNotificationView && self._bulletinNotificationView.get('model').id == bulletin.id) {
            self._bulletinNotificationView.destroy();
         }

         var dialogViewName = bulletin.ui_view;
         if (dialogViewName) {
            self._bulletinDialogView = App.gameView.addView(App[dialogViewName], { uri: bulletin.__self });
         } else {
            self.markBulletinHandled(bulletin);
         }
      },

      toggleListView: function() {
         var self = this;

         // toggle the view
         if (!self._bulletinListView || self._bulletinListView.isDestroyed) {
            self._bulletinListView = App.gameView.addView(App.StonehearthBulletinList, { context: self._orderedBulletins });
         } else {
            self.hideListView();
         }
      },

      hideListView: function() {
         var self = this;
         if (self._bulletinListView != null && !self._bulletinListView.isDestroyed) {
            self._bulletinListView.destroy();
         }
         self._bulletinListView = null;
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
         self._tryShowNextBulletin();
      },

      recreateDialogView: function(bulletin) {
         var self = this;
         if (self._bulletinDialogView) {
            var oldDialogView = self._bulletinDialogView
            // defer the destruction of the old dialog until after render
            // to prevent flickering between views...
            Ember.run.scheduleOnce('afterRender', this, function() {
               oldDialogView.destroy()
            });
            self._bulletinDialogView = null;
            self.showDialogView(bulletin);
         }
      },

      markBulletinHandled: function(bulletin) {
         var self = this;
         var bulletins = self._orderedBulletins;

         // if this is the last bulletin, auto close the list view
         if (bulletins.length <= 1) {
            if (bulletins.length == 0 || bulletins[0].id == bulletin.id) {
               self.hideListView();
            }
         }

         if (bulletin.close_on_handle) {
            radiant.call('stonehearth:remove_bulletin', bulletin.id);
         }
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

      getTrace: function() {
         return this._bulletinBoardTrace;
      }
   });
})();
