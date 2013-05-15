radiant.views['stonehearth.views.sidebar'] = radiant.View.extend({
   options: {
      position: {
         my: "left top",
         at: "left top"
      }
   },

   imagePath: "views/sidebar/images/",

   onAdd: function (view) {
      this._super(view);

      this.currentTab = null;
      this.currentTabController = null;
      
      var self = this;

      this.addTab("store");
      this.addTab("build");

      // event handlers
      this.my(".sidebarNav")
         .hover(function (e) {
            // switch to hover image
            if ($(this).hasClass("sidebarNavActive")) {
               return;
            }
            var tab = $(this).attr("href");
            var img = $(this).find("img");
            console.log(tab);
            img.attr("src", self.imagePath + "tab-" + tab + "-hover.png");
         }, function (e) {
            // switch to normal image
            if ($(this).hasClass("sidebarNavActive")) {
               return;
            }

            var tab = $(this).attr("href");
            var img = $(this).find("img");
            img.attr("src", self.imagePath + "tab-" + tab + ".png");
         })
         .click(function (e) {
            e.preventDefault();
            var tab = $(this).attr("href");
            self.selectTab(tab);
         });
      
      // initial UI state
      this.my("#sidebar").css("height", document.body.clientHeight);
      this.my("[id^=sidebarTab]").hide();

      if (radiant.test) {
         this.selectTab("store");
      }

      this.initHandlers();

      this.my("#sidebarNav").hide();
      this.my("#sidebar").css({ opacity: 0.0 });
   },

   initHandlers: function() {
      var self = this;
      $(top).on("stonehearth.selectTab", function (_, tabId) {
         self.selectTab(tabId, true);
      });
   },

   addTab: function (type, controller) {
      radiant.views.add("stonehearth.views.sidebar." + type);
   },

   refresh: function (data) {
      this.data = data;
   },


   selectTab: function (tabId, noToggle) {
      var self = this;
      var tab = this.my("#sidebarTab-" + tabId);
      var controller = radiant.views.controllers["stonehearth.views.sidebar." + tabId];


      // show the correct tab
      if (this.currentTab && this.currentTab.attr("id") == tab.attr("id")) {
         if (noToggle) {
            // don't toggle the tab off, so selecTab is a noop
            return;
         }

         // toggle the current tab off
         this._hideAndNotifyTab(this.currentTab, this.currentTabController);

         this.currentTab = null;
         this.currentTabController = null;

      } else {
         // show the tab and hide the current tab
         this._showAndNotifyTab(tab, controller);
         this._hideAndNotifyTab(this.currentTab, this.currentTabController);
         
         // set the current tab
         this.currentTab = tab;
         this.currentTabController = controller;
      }

      // show the correct tab nav button
      this.my(".sidebarNav").each(function (e) {
         $(this).removeClass("sidebarNavActive");
         var img = $(this).find("img");
         img.attr("src", self.imagePath + "tab-" + $(this).attr("href") + ".png");
      });

      if (tab) {
         var a = this.my("#sidebarNav").find("[href='" + tabId + "']");
         var img = a.find("img");

         a.addClass("sidebarNavActive");
         img.attr("src", self.imagePath + "tab-" + a.attr("href") + "-active.png");
      }

   },

   _showAndNotifyTab: function (tab, controller) {
      if (tab && controller) {
         console.log("show " + tab.attr("id"));
         //tab.css({ opacity: 1 });
         tab.show();
         if (controller && controller.onShow != undefined) {
            controller.onShow();
         }
      }
   },

   _hideAndNotifyTab: function (tab, controller) {
      if (tab && controller) {
         console.log("hide " + tab.attr("id"));
         //tab.css({ opacity: 0 });
         tab.hide();
         if (controller.onHide != undefined) {
            controller.onHide();
         }
      }
   },

});
