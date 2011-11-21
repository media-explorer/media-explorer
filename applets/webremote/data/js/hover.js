var selected_postfix;
selected_postfix = "-selected.png";

$(document).ready (function () {

$("#remote img, #search img").bind ("vmousedown", function (){
        /* Switch the image src to the selected version */
        var new_src = $(this).attr ('src');
        if (new_src.match (selected_postfix))
          return;

        new_src = new_src.replace (".png", "") + selected_postfix;
        $(this).attr ("src", new_src);

        /* If we're pressing the buttons change the background of the fiveway
         * for added visual feedback.
         */
        if ($(this).hasClass('button-up'))
          {
            $('table.fiveway').css ('background-image',
                                    'url("images/remote-button-background-up-selected.png")');
          }
        else if ($(this).hasClass('button-down'))
          {
            $('table.fiveway').css ('background-image',
                                    'url("images/remote-button-background-down-selected.png")');
          }
        else if ($(this).hasClass('button-left'))
          {
            $('table.fiveway').css ('background-image',
                                    'url("images/remote-button-background-left-selected.png")');
          }
        else if ($(this).hasClass('button-right'))
          {
            $('table.fiveway').css ('background-image',
                                    'url("images/remote-button-background-right-selected.png")');
          }
});


$("#remote img, #search img").bind ("vmouseup vmousecancel mouseleave touchleave", function () {
        var new_src;
        new_src = $(this).attr ('src').replace (selected_postfix, ".png");
        $(this).attr ('src', new_src);

        /* Make sure we put the non selected background back */
        if ($(this).attr ('class').match ("button"))
        {
         $('table.fiveway').css ('background-image',
                                'url("images/remote-button-background.png")');
        }
});


}); /* end document ready */
