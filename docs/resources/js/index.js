function openPage(pageName) {
    // Hide all elements with class="tabheader" or "tabcontent" by default */
    var i, tabheader, tabcontent, tablinks;

    tabheader = document.getElementsByClassName("tabheader");
    for (i = 0; i < tabheader.length; i++) {
        tabheader[i].style.display = "none";
    }

        tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Show the specific content
    document.getElementById(pageName+"Content").style.display = "block";
    document.getElementById(pageName+"Header").style.display = "block";
}

// Get the element with id="defaultOpen" and click on it
document.getElementById("aboutOpen").click();
