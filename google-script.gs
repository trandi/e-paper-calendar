// This goes into a Google Apps Script project and then PUBLISHED as an Web App

// When this is deployed as a Webapp a HTTP GET request will be routed here
function doGet(e) {
  var cal = CalendarApp.getCalendarById('***@group.calendar.google.com');
  
  if (cal == undefined) {
    console.info("no access to calendar");
    return ContentService.createTextOutput('no access to calendar');
  }
 
  const now = new Date();
  var start = new Date(); start.setHours(0, 0, 0); // start at midnight
  const oneday = 24*3600000; // [msec]
  const stop = new Date(start.getTime() + oneday);
  
  var events = cal.getEvents(start, stop);
  var str = '';
  for (var ii = 0; ii < events.length; ii++) {
    var event=events[ii];
    str += event.getStartTime().getHours().pad(2) + event.getStartTime().getMinutes().pad(2) + ' ' + event.getTitle() + '\n';
    var myStatus = event.getMyStatus(); 
  }
  
  console.info(str);
  
  return ContentService.createTextOutput(str);
}


// padding numbers with '0's in front
Number.prototype.pad = function(size) {
  var s = String(this);
  while (s.length < (size || 2)) {s = "0" + s;}
  return s;
}
