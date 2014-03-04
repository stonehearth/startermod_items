$(document).ready(function(){
   radiant.call('autotest:ui:connect_browser_to_client')
            .progress(function (data) {
               var cmd = data[0];
               console.log('automation hook got ', data);
               
               if (cmd == 'CLICK_DOM_ELEMENT') {
                  var jqselector = data[1];
                  $(jqselector)[0].click();
               }
            });
});
