
function CLASS_NAME_HERE() {
   this.myElement = null;
   this.data = null;

   this.init = function (element) {
      var me = this;
      this.myElement = element;
      var v = this.myElement;
   };

   this.refresh = function (data) {
      this.data = data;
   }
}




