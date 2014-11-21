   App.View = Ember.View.extend({

   //the components to retrieve from the object
   components: {},

   init: function() {
      // close all other windows with the "exclusive" class set. Only one such window
      // can be on screen at one time
      this._closeOpenExclusiveWindows();
      this._super();
   },

   destroy: function() {
      this._destroyRootTrace();
      this._destroyRadiantTrace();
      this._unbindHotkeys();
      
      if (this.modalOverlay) {
         this.modalOverlay.destroy();
      }

      //Note: some views have no template, ignore
      if (this.templateName != null) {
         // radiant.call('radiant:send_design_event', 
         //             'ui:hide_ui:' + this.templateName);
      }
      
      // if there's an input on the view, unconditionally re-enable
      // camera movement, in case the input handler code didn't do it
      // properly.
      if (this.$('input')) {
         radiant.call('stonehearth:enable_camera_movement', true)
         radiant.keyboard.enableHotkeys(true);
      }

      // if we're on the mobal stack, remove us from it
      var index = App.stonehearth.modalStack.indexOf(this)
      if (index > -1) {
         App.stonehearth.modalStack.splice(index, 1);
      }

      this._super();
   },

   didInsertElement: function() {
      var position = this.get('position');

      if (position) {
         this.$().children().position(position);
      }

      if (this.$()) {
         this._addHotkeys();   
      }
   },

   show: function() {
      this.set('isVisible', true);
   },

   hide: function() {
      this.set('isVisible', false);
   },

   invokeDestroy: function() {
      if (this.state == 'preRender') {
         Ember.run.scheduleOnce('afterRender', this, this.destroy);
      } else {
         this.destroy();
      }
   },

   _closeOpenExclusiveWindows: function() {
      var self = this;

      $('.exclusive').each(function(i, el) {
         var view = self._getClosestEmberView($(el));

         if (view) {
            view.destroy();
         }
      })
   },

   _addHotkeys: function() {
      this.$('[hotkey]').each(function() {
         var el = $(this);
         var hotkey = el.attr('hotkey');
         $(document).bind('keyup', hotkey, function() {
            el.click();
         })
      });
   },

   _destroyRadiantTrace: function() {
      if (this._radiantTrace) {
         this._radiantTrace.destroy();
         this._radiantTrace = null;
      }
   },

   _destroyRootTrace: function() {
      if (this._rootTrace) {
         if (this._rootTrace.destroy) {
            this._rootTrace.destroy();
         }
         this._rootTrace = null;
      }
   },

   _unbindHotkeys: function() {
      // xxx, todo. 
   },

   _setRootTrace: function(trace) {
      var self = this;

      this._destroyRootTrace();
      this._rootTrace = trace;

      if (this._rootTrace) {
         var firstUpdate = true;
         this._rootTrace.progress(function(eobj) {
               console.log("setting view context for " + self.elementId + " to...", eobj);

               var modelProperty = self.uriProperty ? self.uriProperty : 'context';
               self.set(modelProperty, eobj);

               if (firstUpdate && self.onRenderedUri) {
                  firstUpdate = false;
                  Ember.run.scheduleOnce('afterRender', self, self.onRenderedUri);
               }
            });
      } else {
         // Clear out the context by setting all it's properties to undefined.  The actual
         // `context` variable is fucking magic and can't be fucked with, so this code
         // which used to work in Ember 1.5.1 is verboten ==> self.set('context', {});
         var context = self.get('context');
         radiant.each(context, function(key, value) {
            context.set(key, undefined);
         });
      }
   },

   // `trace` is an object returned from radiant.trace(), radiant.call(), etc.
   _updatedTrace : function() {
      this._destroyRadiantTrace();

      var trace = null;
      if (this._rootTrace) {
         console.log("setting view context to deferred object");
         var radiantTrace = new RadiantTrace()
         radiantTrace.userTrace(this.trace, this.components);
      }
      this._setRootTrace(trace);
   }.observes('trace'),

   // `uri` is a string that's valid to pass to radiant.trace()
   _updatedUri: function() {
      this._destroyRadiantTrace();

      var trace = null;
      if (this.uri) {
         console.log("setting view context for " + this.uri);
         this._radiantTrace = new RadiantTrace()
         trace = this._radiantTrace.traceUri(this.uri, this.components);
      }
      this._setRootTrace(trace);
   }.observes('uri'),

   _mapToArray: function(mapPath, arrayPath) {
      
      //var vals = [];
      var map = this.get(mapPath);
      var vals = this._mapToArrayObject(map);
      /*
      if (map) {
         $.each(map, function(k ,v) {
            if(k != "__self" && map.hasOwnProperty(k)) {
               vals.push(v);
            }
         });
      }
      */
      this.set(arrayPath, vals);
   },

   _mapToArrayObject: function(map) {
      var vals = [];     
      if (map) {
         $.each(map, function(k ,v) {
            if(k != "__self" && map.hasOwnProperty(k)) {
               vals.push(v);
            }
         });
      }
      return vals;
   },

   _getClosestEmberView: function(el) {
     var id = el.closest('.ember-view').attr('id');
     if (!id) return;
     if (Ember.View.views.hasOwnProperty(id)) {
       return Ember.View.views[id];
     }
   },

});
