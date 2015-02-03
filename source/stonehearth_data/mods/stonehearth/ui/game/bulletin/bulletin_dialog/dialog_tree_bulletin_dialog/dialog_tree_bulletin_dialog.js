App.StonehearthDialogTreeBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'dialogTreeBulletinDialog',


   didInsertElement: function() {
      this._super();
      this._wireButtonToCallback('#collectionPayButton',    'collection_pay_callback');
      this._wireButtonToCallback('#collectionCancelButton', 'collection_cancel_callback');
   },

   _choices: function() {
   	var buttons = [];
      var choices = this.get('model.data.choices');
      if (choices) {
      	buttons = radiant.map_to_array(choices, function(k, v) {
            return {
               id: k,
               text: i18n.t(k),
            }
         });
      }
      this.set('buttons', buttons);
   }.observes('model.data.choices'),
   
   actions: {
      choose: function(choice) {
         var bulletin = this.get('model');
         var instance = bulletin.callback_instance;
         radiant.call_obj(instance, '_on_dialog_tree_choice', choice);
      }
   },

});
