$(document).ready(function(){
   radiant.call('autotest:ui:connect_browser_to_client')
            .progress(function (data) {
               var cmd = data[0];
               console.log('automation hook got ', data);
               
               if (cmd == 'CLICK_DOM_ELEMENT') {
                  var jqselector = data[1];
                  var element = $(jqselector)[0];
                  if (!element) {
                     console.error('could not find element matching selector: ', jqselector)
                  }
                  element.click();
               }
            });
});
