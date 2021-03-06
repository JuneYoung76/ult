var app = require('http').createServer(handler);
var io = require('socket.io')(app);
var url = require('url');
var fs = require('fs');

//This will open a server at localhost:5000. Navigate to this in your browser.
app.listen(5000);

// Http handler function
function handler (req, res) {

    // Using URL to parse the requested URL
    var path = url.parse(req.url).pathname;

    // Managing the root route
    if (path == '/') {
        index = fs.readFile(__dirname+'/public/index.html', 
            function(error,data) {

                if (error) {
                    res.writeHead(500);
                    return res.end("Error: unable to load index.html");
                }

                res.writeHead(200,{'Content-Type': 'text/html'});
                return res.end(data);
            });

    // Managing the route for the javascript files
    } else if( /\.(js)$/.test(path) ) {
        index = fs.readFile(__dirname+'/public'+path, 
            function(error,data) {

                if (error) {
                    res.writeHead(500);
                    return res.end("Error: unable to load " + path);
                }

                res.writeHead(200,{'Content-Type': 'text/plain'});
                return res.end(data);
            });

    } else if( /\.(jpeg)$/.test(path) ) {
        index = fs.readFile(__dirname+'/public'+path,
              function(error,data) {
                  if (error) {
                      res.writeHead(500);
                      return res.end("Error: unable to load " + path);
                  }

                  res.writeHead(200,{'Content-Type': 'image/jpeg'});
                  return res.end(data);
              });
    } else {
        res.writeHead(404);
        res.end("Error: 404 - File not found.");
    }

}

// Web Socket Connection
io.sockets.on('connection', function (socket) {
    var running = true;
    console.log("websocket connecting..");

    socket.on("set", function(msg) {
        console.log(msg);
        var conf_str = msg["max_distance"] + ", " + msg["max_speed"];
        fs.writeFile("/tmp/ult.conf", conf_str, function(err) {
            if(err) {
                return console.log(err);
            }
            console.log("The file was saved!");
        });

    });

    setInterval(function(){
        fs.readFile('/tmp/ult.log', 'utf8', function (err,data) {
            if (err) {
                return console.log(err);
            }
            var arr = data.split(",");
            console.log(arr);
            socket.emit("info", {num_photos: Number.parseInt(arr[0]),
                                 distance: Number.parseInt(arr[1]),
                                 speed: Number.parseInt(arr[2])
                                });
        });
    }, 1000);

});
