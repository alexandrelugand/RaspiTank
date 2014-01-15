function Log(msg) {
    $("#Logger").append(msg + "\n");
    $('#Logger').scrollTop($('#Logger')[0].scrollHeight);
}