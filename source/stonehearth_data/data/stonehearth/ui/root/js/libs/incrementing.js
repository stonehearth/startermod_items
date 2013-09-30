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

    var $button = $(this),
        oldValue = $button.parent().find("input").val(),
        $input = $button.parent().find("input"),
        newVal, inputMax, inputMin;

    if ($button.text() == "+") {
      inputMax = $input.attr('max');
      if (oldValue < inputMax) {
        newVal = parseFloat(oldValue) + 1;
      } else {
        newVal = inputMax;
      }
  	} else {
      inputMin = $input.attr('min');
      if (oldValue > inputMin) {
        newVal = parseFloat(oldValue) - 1;
	    } else {
        newVal = inputMin;
      }
	  }

    $input.val(newVal);

  });

}