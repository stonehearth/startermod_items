App.Route = Ember.Route.extend({
	
});

App.Route.reopenClass({

  buildRoutes: function(builder) {
    var oldBuilder = App.routeBuilder;
    App.routeBuilder = function() {
      if (oldBuilder) oldBuilder.call(this);
      return builder.call(this);
    };
  },

  /**
    Shows a modal. From Discourse code.

    @method showModal
  **/
  showModal: function(router, name, model) {
    router.controllerFor('modal').set('modalClass', null);
    router.render(name, {into: 'modal', outlet: 'modalBody'});
    var controller = router.controllerFor(name);
    if (controller) {
      if (model) {
        controller.set('model', model);
      }
      controller.set('flashMessage', null);
    }
  }

});

App.RemoteObjectRoute = App.Route.extend({
  setupController: function (controller, model) {
    var m = App.RemoteObject.create();
    m.get(model.id);
    
    controller.set("model", m);
  }
});
