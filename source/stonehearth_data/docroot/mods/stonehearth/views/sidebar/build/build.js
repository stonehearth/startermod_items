radiant.views['stonehearth.views.sidebar.build'] = radiant.View.extend({
   options: {
      position: {
         my: "left top",
         at: "left top",
         of: "#sidebarTab-build"
      },
      deferShow: false
   },

   imagePath: "views/sidebar/build/images/",

   onAdd: function (view) {
      this._super(view);
      this.house = this.my("#sidebarHouseContainer");
      this.currentToolbox == null;
      this.currentTool = null;
      var container = this.my("#buildMode");

      container.width($(window).width() - 20);
      console.log("offset: " + container.offset().left);

      this.my(".button").button();

      //initial UI state
      this.my(".toolbox").hide();

      this.setupHouseImageMap();
      this.initHandlers();

      //this.selectToolbox("walls");
   },

   initHandlers: function () {
      var self = this;

      // Handlers for groups of build tools
      this.my(".rightTool").click(function (e) {
         self.selectTool($(this));
      });

      // Per tool handler
      this.my("#toolSelect").click(function (e) {
         if ($(this).is(":visible")) {
            radiant.api.cmd('select');
         }
      });

      this.my("#toolDirectSelect").click(function (e) {
         if ($(this).is(":visible")) {
            radiant.api.cmd('direct_select');
         }
      });
   },

   onShow: function () {
      radiant.api.cmd('set_view_mode', { 'mode': 'build' });
      if (this.currentTool) {
         this.selectTool(this.currentTool);
      } else {
         this.selectTool(this.my(".defaultTool"));
      }
   },

   onHide: function () {
      this.clearTool();
      radiant.api.cmd('set_view_mode', { 'mode': 'game' });
   },

   addPalette: function (id, type) {
      var controller = new StonehearthBuildModePalette();
      radiant.views.addViewAndController("/views/sidebar/palette", controller,
         {
            of: this.my(id),
            my: "left top",
            at: "left top"
         });

      return controller;
   },

   setupHouseImageMap: function () {
      var self = this;

      var map = this.my("[id^=sidebarMap]");
      var hoverImg, selectedImg;

      map.find("area").on({
         mouseenter: function () {
            var part = $(this).attr("href");
            hoverImg = ( part == "stairs" || part == "floor" || part == "fence" ? self.my("#sidebarFloorHover") : self.my("#sidebarHouseHover"));
            hoverImg.attr("src", self.imagePath + "construction/" + part + "-active.png");
            console.log(hoverImg);
            console.log(self.imagePath + part + "-active.png");
            //this.style.cursor = 'wait';
            console.log(part);
         },
         mouseleave: function () {
            hoverImg.attr("src", self.imagePath + "clear.png");
         },
         click: function (e) {
            e.preventDefault();
            var part = $(this).attr("href");
            self.selectToolbox(part);
         }
      });

      this.my("#sidebarHouseSelectorPopped").hide();

      // Crazy hack to fix the cursor for image maps
      // http://jsfiddle.net/jamietre/M6jBp/
      /*
      this.my("#sidebarHouseSelector").mapster({
         onMouseover: function () {
            console.log("mouse over!");
            console.log($(this).parent());
            $("#wrap").css('cursor', 'wait');
         }
      });
      */
   },

   selectToolbox: function (part) {
      console.log("select toolbox:" + part);
      var toolbox = this.my("#" + part);

      if (this.currentToolbox) {
         this.currentToolbox.hide();
      }

      toolbox.show();

      this.currentToolbox = toolbox;
      this.updateHouse(part);
   },

   updateHouse: function (part) {
      // show the right selected images if a new part has been specified
      this.my(".sidebarPartSelected").attr("src", this.imagePath + "clear.png");
      var selectedImg;

      if (part == "floor" || part == "stairs" || part == "fence") {
         selectedImg = this.my("#sidebarFloorSelected");
         if (part == "floor" || part == "stairs") {
            this.houseUp();
         }
      } else {
         selectedImg = this.my("#sidebarHouseSelected");
         this.houseDown();
      }

      selectedImg.attr("src", this.imagePath + "construction/" + part + "-active.png");

   },

   houseUp: function () {
      this.moveHouse("-80px");
   },

   houseDown: function () {
      this.moveHouse("0px");
   },

   houseIsUp: function () {
      return this.house.css("top") == "-80px";
   },

   moveHouse: function (newTop) {
      var self = this;

      this.house.animate({
         top: newTop
      }, 150, function () {
         if (self.houseIsUp()) {
            self.my("#sidebarHouseSelectorPopped").show();
            self.my("#sidebarHouseSelector").hide();
         } else {
            self.my("#sidebarHouseSelectorPopped").hide();
            self.my("#sidebarHouseSelector").show();
         }
      });
   },


   clearTool: function() {
      radiant.api.cmd('select_tool', { 'tool': 'none' });
      this.currentTool = null;
   },

   selectTool: function (element) {
      // make sure the toolbox is visible, XXX, do this in an AOP way so we 
      // don't have to copy/paste this code everywhere.
      var sidebar = radiant.views.controllers["stonehearth.views.sidebar"];

      // switch to the build UI if necessary
      if (!this._myElement.is(":visible")) {
         sidebar.selectTab("build");
      }

      if (this.currentToolbox) {
         this.currentToolbox.find(".rightTool").removeClass("activeTool");
      }

      element.addClass("activeTool");

      // switch to the coorect toolbox
      this.selectToolbox(element.parent().attr("id"));

      // send the command to activate the tool
      var tool = element.attr("tool");
      console.log(">>" + tool);

      radiant.api.cmd('select_tool', { 'tool': tool });
      this.currentTool = element;
   }

});
