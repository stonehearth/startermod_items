$(document).ready(function(){

   $('body').on( 'focus', 'input', function() {
      radiant.call('stonehearth:enable_camera_movement', false);
      radiant.keyboard.enableHotkeys(false);
   });      

   $('body').on( 'blur', 'input', function() {
      radiant.call('stonehearth:enable_camera_movement', true)
      radiant.keyboard.enableHotkeys(true);
   });      

   /* Use with numeric input and buttons housed inside a span, like this:
     <span>
       <div class="dec numericButton">-</div>
       <input type="number" name="quantityToMake" id="makeNumSelector" value="1" min="1" max="99" class="craftNumericSelect">
       <div class="inc numericButton">+</div>
     </span>
   */
  //Note: input is a numeric input
  $('body').on( 'focusout', ':input[type="number"]', function() {
    var inputMax = $(this).attr('max'),
        inputMin = $(this).attr('min'),
        currValue = $(this).val();

    if (currValue > inputMax) {
      currValue = inputMax;
    } else if (currValue < inputMin) {
      currValue = inputMin;
    }

    $(this).val(currValue);
  });



  $('body').on( 'click', '.numericButton', function() {
    var button = $(this);
    var inputControl = button.parent().find('input');
    var oldValue = parseInt(inputControl.val());
    var newVal, inputMax, inputMin;

    if (inputControl.prop('disabled')) {
      return;
    }


    if (button.text() == "+") {
      inputMax = parseInt(inputControl.attr('max'));
      if (oldValue < inputMax) {
        newVal = oldValue + 1;
      } else {
        newVal = inputMax;
      }
   } else {
      inputMin = parseInt(inputControl.attr('min'));
      if (oldValue > inputMin) {
        newVal = parseInt(oldValue) - 1;
       } else {
        newVal = inputMin;
      }
     }

    inputControl.val(newVal).change();
  });

});
