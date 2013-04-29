radiant.View = Class.extend({
   _myElement: null,

   init: function () {
      this.type = "view";
   },

   onAdd: function (view) {
      this._myElement = view;
   },

   render: function (selector, value) {
      this.my(selector).html(value);
   },

   /*
   render: function (templateSelector, data) {
      this.my(templateSelector)
      return Mustache.render(template, data);
   },
   */

   my: function (selector) {
      return this._myElement.find(selector);
   },

   ready: function () {
      this._myElement.removeClass("ui-loading");
   }

});

radiant.Dialog = radiant.View.extend({
   
   _defaults: {
      dialogOptions: {
         buttons: [{ text: "Ok", click: function () { console.log(this); this._onOk; } }]
      }
   },

   init: function () {
      this._super();

      this.type = "dialog";

      var self = this;
      var defaults = {
         dialogOptions: {
            /*
            buttons: [{
               text: "Ok", click: function () {
                  self.onOk();
               }
            }]
            */
         }
      }

      this.options = $.extend(true, this.options, defaults);
   },

   dialog: function () {
      return this._myElement.parent();
   },

   onOk: function () {
      this._myElement.dialog("close");
   },

   ready: function () {
      var dialog = element.parents(".ui-loading");
      if (dialog.length > 0) {
         dialog.removeClass("ui-loading");
      }
   }

});

radiant.views = {

   controllers: {},

   add: function (name) {
      this.add_(name.replace(/\./g, '/'));
   },

   add_: function (url) {

      // Load the view's css
      this._loadCss(url + "/style.css");

      // Load the script
      var viewName = this._getViewName(url);
      var scriptUrl = url + "/" + viewName + ".js";

      scriptUrl = '../' + scriptUrl; // xx UGGGGGGGG
      this._loadScript(scriptUrl, function () {
         var namespace = url.replace(/\//g, '.')
         console.log("new " + namespace + "()");
         var controller = new radiant.views[namespace]();

         url = '../' + url; // xx UGGGGG
         radiant.views.controllers[namespace] = controller;
         radiant.views.addViewAndController(url, controller);
      });
   },

   addViewAndController: function (url, controller) {
      console.log("adding " + url);

      var options = controller.options;
      if (options == undefined) {
         options = {}
      }

      if (options.position.of == undefined) {
         options.position.of = $('body');
      } else {
         options.position.of = $(options.position.of);
      }

      var viewName = this._getViewName(url);
      var viewId = "radiant-views-" + viewName;

      // add the view
      var viewClass = controller.type == "view" ? "radiantView" : "";
      var newView = $('<div class=' + viewClass + ' id=' + viewId + ' ></div>')
         .load(url, function () {
            console.log("initializing " + url);

            if (options.deferShow) {
               newView.addClass('ui-loading');
            }

            controller.onAdd(newView);

            if (controller.type == "view") {
               $(this).position(options.position);
            }
         });

      if (controller.type  == "dialog") {
         //options.dialogOptions.dialogClass = "ui-loading";
         options.closeOnEscape = true;

         newView.dialog(options.dialogOptions);

         // enable full window drag
         //newView.dialog({ draggable: false }).parent().draggable();
      } else {
         newView.appendTo(options.position.of);
         if (options.hidden) {
            newView.hide();
         }
      }

   },

   _getViewName: function (path) {
      var parts = path.split("/");
      return parts[parts.length - 1];
   },

   _loadCss: function (url) {
      url = '../' + url; // xxx UGGGGGG
      var link = $("<link>");
      link.attr({
         type: 'text/css',
         rel: 'stylesheet',
         href: url
      });
      $("head").append(link);
   },

   _loadScript: function (url, onLoad) {
      var script = document.createElement('script');
      script.type = 'text/javascript';
      script.src = url;
      script.onload = onLoad;
      script.onerror = function () {
         throw ('failed to load script at ' + url);
      };
      document.getElementsByTagName('head')[0].appendChild(script);
   },



   addScript: function (url) {
      $("head").append('<script type="text/javascript" src="' + url + '"></script>');
   },

   showView: function (id) {
      var view = $("#" + id);
      view.show();
   },

   hideView: function (id) {
      var view = $("#" + id);
      view.hide();
      //view.css("z-index", 99);
   },

   getFocus: function () {
      if (this._focus.length > 0) {
         var f = this._focus[this._focus.length - 1];
         return f;
      } else {
         return null;
      }
   },

   setFocus: function (id) {
      console.log('focus set to ' + id);
      this._focus.push(id);
      $("#" + id).css("z-index", this._z++)

   }

}