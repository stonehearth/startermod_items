App.View = Ember.View.extend({

  //the interface to set the remote id. Call this using the set method of the view;
  //view.set('remoteId', entity.id)
  remoteId: 0,

  //the components to retrieve from the object
  components: [],

  _setRemoteId: function() {
    
    var m = App.RemoteObject.create({
      id: this.remoteId,
      components: this.components
    });

    this.set('context', m);
  }.observes('remoteId'),



});
