function StonehearthSidebarBuild() {
   this.myElement = null;
   this.data = null;
   this.imagePath = "/css/default/images/views/sidebar/";
   this.house = null;

   this.currentSubtab;

   this.init = function (element) {
      radInfo("Sidebar build pane initializing");

      var me = this;
      this.myElement = element;
      var v = this.myElement;

      this.house = v.find("#sidebar-house-container");

      // Build the UI
      this.addPalette(v.find("#sidebar-roof-palette"), "roof");
      this.addPalette(v.find("#sidebar-wall-palette"), "wall");
      this.addPalette(v.find("#sidebar-floor-palette"), "floor");
      this.addPalette(v.find("#sidebar-doors-palette"), "furniture");
      this.addPalette(v.find("#sidebar-furniture-palette"), "furniture");

      // Event handlers
      v.find(".sidebar-tool").click(function (e) {
         me.selectTool($(this));
      });


      // Initial UI state
      this.setupHouseImageMap();
      this.selectSubtab("walls");
   };

   this.addPalette = function (element, type) {
      var controller = new StonehearthBuildModePalette();
      radiant.views.addViewAndController("/views/sidebar/palette", controller,
         {
            of: element,
            my: "left top",
            at: "left top"
         });

      return controller;
   }

   this.setupHouseImageMap = function () {
      var me = this;
      var v = this.myElement;

      // playing with image maps
      var map = v.find("[id^=sidebar-map-]");
      var hoverImg, selectedImg;

      map.find("area").hover(
         function () {
            var part = $(this).attr("href");

            if (part == "bed" || part == "floor") {
               hoverImg = v.find("#sidebar-floor-hover");
            } else {
               hoverImg = v.find("#sidebar-house-hover");
            }

            hoverImg.attr("src", me.imagePath + part + "-hover.png");
         },
         function () {
            hoverImg.attr("src", me.imagePath + "clear.png");
         }
      ).click(
         function (e) {
            e.preventDefault();
            var part = $(this).attr("href");

            var tab = $(this).parent();

            while (tab.attr("id").indexOf("sidebar-tab-") != 0) {
               tab = tab.parent();
            }

            var prefix = "sidebar-tab-";
            var tabId = tab.attr("id").substring(prefix.length);
            me.selectSubtab(part);
         }
      );

      v.find("#sidebar-house-selector-popped").hide();
   }

   this.selectSubtab = function (part) {
      var me = this;
      var v = this.myElement;
      var subtab;

      // show the correct subtab
      subtab = v.find("#sidebar-subtab-" + part);
      this.currentSubtab = subtab;

      v.find("[id^=sidebar-subtab-]")
         .removeClass("sidebar-selected-subtab")
         .hide();

      subtab
         .addClass("sidebar-selected-subtab")
         .show();

      // show the right selected images if a new part has been specified
      v.find(".sidebar-part-selected").attr("src", this.imagePath + "clear.png");

      if (subtab.hasClass("sidebar-house-up")) {
         selectedImg = v.find("#sidebar-floor-selected");
         this.houseUp();
      } else {
         selectedImg = v.find("#sidebar-house-selected");
         this.houseDown();
      }

      selectedImg.attr("src", this.imagePath + part + "-hover.png");
   }

   this.moveHouse = function (newTop) {
      var v = this.myElement;
      var me = this;

      this.house.animate({
         top: newTop
      }, 150, function () {
         if (me.houseIsUp()) {
            v.find("#sidebar-house-selector-popped").show();
            v.find("#sidebar-house-selector").hide();
         } else {
            v.find("#sidebar-house-selector-popped").hide();
            v.find("#sidebar-house-selector").show();
         }
      });
   }

   this.houseUp = function () {
      this.moveHouse("-100px");
   }

   this.houseDown = function () {
      this.moveHouse("0px");
   }

   this.houseIsUp = function () {
      return this.house.css("top") == "-100px";
   }

   this.selectTool = function (element) {
      var v = this.myElement;

      this.currentSubtab.find(".sidebar-tool").removeClass("sidebar-active-tool");
      element.addClass("sidebar-active-tool");
   }

}




