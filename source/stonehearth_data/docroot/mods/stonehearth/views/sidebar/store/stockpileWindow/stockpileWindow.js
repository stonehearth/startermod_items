radiant.views['stonehearth.views.sidebar.store.stockpileWindow'] = radiant.Dialog.extend({
   options: {
      uiType: "dialog",
      dialogOptions: {
         title: "Stockpile",
         width: 600,
         height: 350,
         modal: true
      },
      position: {
         my: "center",
         at: "center"
      },
      resizable: false
   },

   onAdd: function (element) {
      this._super(element);

      
      this.my(".namefield").focus();

      this.initHandlers();
      this.ready();
   },

   initHandlers: function () {
      var self = this;

      this.my(".namefield").keydown(function (e) {
         if (e.keyCode == 13) {
            $(this).blur();
            self.dialog().find('.ui-dialog-buttonpane button:eq(0)').focus();
            return false;  //don't propogate up the DOM
         }
      });
   },

   onOk: function () {
      this._super();
   }

});
