function RadiantFormBuilder() {

   this.buildForm = function (container, data) {
      for (var i = 0; i < data.length; i++) {
         var option = data[i];
         var optionTag = option.name;
         if (option.type == "boolean") {
            optionTag = this.booleanOption(option);
         } else if (option.type == "string") {
            if (option.options != undefined) {
               optionTag = this.enumOption(option);
            } else {
               optionTag = this.stringOption(option);
            }
         }

         container.append("<p>" + optionTag + "</p>");
      }
   };

   this.booleanOption = function (option) {
      var checked = option.value == true ? "checked" : "";

      return '<input type="checkbox" name="' + option.id + '" id="' + option.id + '" ' + checked + ' /><label for="' + option.id + '">' + option.name + '</label>';
   };

   this.stringOption = function (option) {
      return '<label for="' + option.id + '">' + option.name + '</label><input type="text" name="' + option.id + '"' + 'value="' + option.value + '">'
   };

   this.enumOption = function (option) {
      if (option.control != undefined && option.control == "radio") {
         return this.enumOptionRadio(option);
      } else {
         return this.enumOptionSelect(option);
      }
   };

   this.enumOptionRadio = function (option) {
      var tag = "";

      for (var i = 0; i < option.options.length; i++) {
         var selected = option.value == option.options[i].value ? "selected" : "";

         tag += '<input type="radio" value="' + option.options[i].value + '" id="' + option.options[i].value + '" name="' + option.id + '">';
         tag += '<label for="' + option.options[i].value + '">' + option.options[i].name + '</label>';
      }

      return tag;
   };

   this.enumOptionSelect = function (option) {
      var tag = '<select name="' + option.id + '">';

      for (var i = 0; i < option.options.length; i++) {
         var selected = option.value == option.options[i].value ? "selected" : "";

         tag += '<option value="' + option.options[i].value + '" ' + selected + '>' + option.options[i].name + '</option>';
      }

      tag += '</select>';

      return '<label for="' + option.id + '">' + option.name + '</label>' + tag;

      //return '<label for="' + option.id + '">' + option.name + '</label><input type="text" id="' + option.id + '"' + 'value="' + option.value + '">'
   };

}