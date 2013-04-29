radiant.keyboard.addKeyHandler(function (e) {
   if (e.keyCode == radiant.keyboard.getKeybind("main_menu")) {
      radInfo("esc!");

      var mainmenuElement = $("#radiant-views-mainmenu"); // XXX abstraction leak!
      if (mainmenuElement.length == 0) {
         radiant.views.addDialog(new StonehearthMainMenu(), {
            title: "Main Menu",
            width: 600,
            height: 600,
            modal: true,
            resizable: false,
            buttons: [
				{
				   text: "Ok",
				   click: function () {
				      $(this).dialog("close");
				   }
				},
				{
				   text: "Cancel",
				   click: function () {
				      $(this).dialog("close");
				   }
				}
            ]
         });
      } else {
         var isOpen = mainmenuElement.dialog("isOpen");
         if (!isOpen) {
            mainmenuElement.dialog("open");
         } else {
            //radiant.views.mainmenu.myElement.dialog("close");
         }
      }
   }
});

function StonehearthMainMenu() {
   this.myElement = null;
   this.url = '/views/main-menu';
   

   this.init = function (element) {
      var me = this;

      radInfo("initializing the main menu: " + element.attr('id'));

      this.myElement = element;

      this.populateOptions();
      this.populateKeys();

      this.myElement.find("#main-menu-tabs").tabs();

   };

   this.populateOptions = function () {
      radInfo('loading options form...');
      var me = this;
      $.getJSON("/conf/options.json", function (data) {
         var fb = new RadiantFormBuilder();

         var optionsElement = me.myElement.find('#main-menu-options');
         fb.buildForm(optionsElement, data);
      });
   };

   this.populateKeys = function () {
      radInfo('loading key bindings...');
      var me = this;

      $.getJSON("conf/keybindings.json", function (data) {
         var bindingsElement = me.myElement.find('#main-menu-keybindings');
         var keys = data.keybindings;

         for (var i = 0; i < data.keygroups.length; i++) {
            var group = data.keygroups[i];

            $('<b>' + group.displayname + '</b><br />').appendTo(bindingsElement);

            for (var j = 0; j < group.keys.length; j++) {
               var keyId = group.keys[j];
               var keyName = data.keybindings[keyId].name;
               var keyBind = String.fromCharCode(data.keybindings[keyId].key);

               if (keyName != "") {
                  var row = $('<div style="display:table-row"></div>');
                  $('<label style="display:table-cell" for=' + keyId + '>' + keyName +
                     '</label><input style="display:table-cell" type=text id=' + keyId +
                     ' value=' + keyBind + ' />').appendTo(row);

                  bindingsElement.append(row);
               }
            }
         }
      });
   }

}
