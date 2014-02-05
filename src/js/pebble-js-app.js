
//function iconFromWeatherId(weatherId) {
//  if (weatherId < 600) {
//    return 2;
//  } else if (weatherId < 700) {
//    return 3;
//  } else if (weatherId > 800) {
//    return 1;
//  } else {
//    return 0;
//  }
//}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  //req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
  //  "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.open('GET', "http://api.openweathermap.org/data/2.5/find?" +
             "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperatureF, temperatureC, icon, city;
        if (response && response.list && response.list.length > 0) {
          var weatherResult = response.list[0];
          temperatureC = Math.round(weatherResult.main.temp - 273.15);
          temperatureF = Math.round(1.8 * (weatherResult.main.temp - 273.15) + 32);
          //icon = iconFromWeatherId(weatherResult.weather[0].id);
          icon = weatherResult.weather[0].main;
          city = weatherResult.name;
          console.log(temperatureF);
          console.log(temperatureC);
          console.log(icon);
          console.log(city);
          Pebble.sendAppMessage({
            "icon":icon,
            "temperatureC":"" + temperatureC,
            "temperatureF":"" + temperatureF
            });
        }

      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "temperatureC":"N/A",
    "temperatureF":"N/A"
  });
}

var initialized = false;
var locationOptions = { "timeout": 150000, "maximumAge": 600000 };


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          initialized = true;
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                          console.log(e.payload.temperatureF);
                          console.log(e.payload.temperatureC);
                          console.log("message!");
                        });

Pebble.addEventListener("showConfiguration",
                        function() {
                            console.log("showing configuration");
                            Pebble.openURL("http://keelanc.github.io/pebble/text_watch_2_cfg_1.html");
                        });

Pebble.addEventListener("webviewclosed",
                        function(e) {
                             console.log("configuration closed");
                             //console.log(e.type);
                             //console.log(e.response);
                             //var config = e.response;
                             var config = JSON.parse(e.response);
                             console.log(config);
                             Pebble.sendAppMessage({
                                "updateConfig":config
                             })
                         });


