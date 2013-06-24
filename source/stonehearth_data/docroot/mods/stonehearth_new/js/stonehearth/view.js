App.View = Ember.View.extend({

   //the interface to set the remote id. Call this using the set method of the view;
   //view.set('remoteId', entity.id)
   url: "",

   //the components to retrieve from the object
   components: [],

   _fetchObject: function() {
    
      var m = App.RemoteObject.create({
         url: this.url,
         components: this.components
      });

      this.set('context', m);
   }.observes('url'),
});
