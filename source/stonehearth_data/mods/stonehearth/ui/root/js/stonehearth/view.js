   App.View = Ember.View.extend({

   //the components to retrieve from the object
   components: {},

   init: function() {
      this._trace = new RadiantTrace();
      this._super();
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();   
         this._trace = null;
      }
      
      if (this.modalOverlay) {
         this.modalOverlay.destroy();
      }

      //Note: some views have no template, ignore
      if (this.templateName != null) {
         radiant.call('radiant:send_design_event', 
                      'ui:hide_ui:' + this.templateName);
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

   _updatedUri: function() {
      var self = this;

      if (this._trace) {
         this._trace.destroy();
      }

      if (this.uri) {
         console.log("setting view context for " + this.uri);
         this._trace = new RadiantTrace()
         this._trace.traceUri(this.uri, this.components)
            .progress(function(eobj) {
               console.log("setting view context for " + self.uri);
               self.set('context', eobj)
            });
      } else {
         self.set('context', {});
      }

   }.observes('uri'),

});
