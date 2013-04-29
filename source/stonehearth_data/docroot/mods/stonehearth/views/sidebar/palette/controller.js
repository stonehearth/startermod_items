
function StonehearthBuildModePalette() {
   this.myElement = null;
   this.popup = null;
   this.data = null;
   this.imagePath = "/css/default/images/views/sidebar/";
   this.materialPath = "/css/default/images/materials/";


   this.init = function (element) {
      radInfo("build mode material palette initialized");

      var me = this;
      this.myElement = element;
      var v = this.myElement;
      this.popup = v.find(".sidebar-palette-popup");

      this.popup.scale9Grid({ top: 20, bottom: 160, left: 40, right: 40 });

      // event handlers
      v.click(function () {
         me.popup.toggle();
      });

      this.popup.click(function (e) {
         e.stopPropagation();
      });

      v.find(".sidebar-palette-popup-item").click(function (e) {
         console.log("material click");
         e.preventDefault();
         me.selectMaterial($(this).attr("href"));
      });

      // initial UI state

   };

   this.refresh = function (data) {
      this.data = data;
   };

   this.hide = function () {
      this.myElement.hide();
      this.popup.hide();
   };

   this.show = function () {
      this.popup.hide();
      this.myElement.show();
   };

   this.selectMaterial = function (id) {
      radInfo("selecting material: " + id);

      v = this.myElement;

      var img = v.find(".sidebar-selected-material");
      img.attr("src", this.materialPath + id + ".png");

      this.popup.hide();
   }
}




