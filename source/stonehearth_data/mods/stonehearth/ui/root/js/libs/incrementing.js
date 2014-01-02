/**
* Use with numeric input and buttons housed inside a span, like this:
  <span>
    <div class="dec numericButton">-</div>
    <input type="number" name="quantityToMake" id="makeNumSelector" value="1" min="1" max="99" class="craftNumericSelect">
    <div class="inc numericButton">+</div>
  </span>
*/

function initIncrementButtons() {

  //Note: input is a numeric input
  $(':input[type="number"]').focusout(function(eventObject) {
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



  $(".numericButton").on("click", function() {
    var button = $(this);
    var inputControl = button.parent().find('input');
    var oldValue = inputControl.val();
    var newVal, inputMax, inputMin;

    if (inputControl.prop('disabled')) {
      return;
    }


    if (button.text() == "+") {
      inputMax = inputControl.attr('max');
      if (oldValue < inputMax) {
        newVal = parseFloat(oldValue) + 1;
      } else {
        newVal = inputMax;
      }
  	} else {
      inputMin = inputControl.attr('min');
      if (oldValue > inputMin) {
        newVal = parseFloat(oldValue) - 1;
	    } else {
        newVal = inputMin;
      }
	  }

    inputControl.val(newVal).change();
  });

}