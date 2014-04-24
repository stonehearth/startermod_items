   App.View = Ember.View.extend({

   //the components to retrieve from the object
   components: {},

   init: function() {
      this._super();      
   },

   destroy: function() {

      this._destroyRootTrace();
      this._destroyRadiantTrace();
      
      if (this.modalOverlay) {
         this.modalOverlay.destroy();
      }

      //Note: some views have no template, ignore
      if (this.templateName != null) {
         radiant.call('radiant:send_design_event', 
                      'ui:hide_ui:' + this.templateName);
      }
      
      // if there's an input on the view, unconditionally re-enable
      // camera movement, in case the input handler code didn't do it
      // properly.
      if (this.$('input')) {
         radiant.call('stonehearth:enable_camera_movement', true)
         radiant.keyboard.enableHotkeys(true);
      }

      this._super();
   },

   didInsertElement: function() {
      var position = this.get('position');

      if (position) {
         this.$().children().position(position);
      }
   },

   my: function(selector) {
      if (!selector) {
         return this.$();
      } else {
         return this.$().find(selector);
      }
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

   _setRootTrace: function(trace) {
      var self = this;

      this._destroyRootTrace();
      this._rootTrace = trace;

      if (this._rootTrace) {
         this._rootTrace.progress(function(eobj) {
               console.log("setting view context for " + self.elementId + " to...");
               console.log(eobj);
               self.set('context', eobj)
            });
      } else {
         self.set('context', {});
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

});
