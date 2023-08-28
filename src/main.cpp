#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
#include <DNSServer.h>

const char *ssid = "RoboHand";
const byte DNS_PORT = 53;
DNSServer dnsServer;

AsyncWebServer server(80);

static const int servosPins[4] = {25, 26, 27, 14};
Servo servos[4];

String getHTML()
{
  return R"html(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Servo Control</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
            }
            #joystickContainer1, #joystickContainer2 {
                width: 200px;
                height: 200px;
                border: 1px solid black;
                position: relative;
                margin: 20px auto;
            }
            .draggable {
                width: 40px;
                height: 40px;
                background-color: red;
                border-radius: 50%;
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
            }
        </style>
    </head>
    <body>
        <h2>ESP32 Servo Control</h2>
        <div id="joystickContainer1">
            <div class="draggable" id="draggable1"></div>
        </div>
        <div id="joystickContainer2">
            <div class="draggable" id="draggable2"></div>
        </div>
        <script>
            function setupJoystick(containerId, draggableId, servoXId, servoYId) {
                let container = document.getElementById(containerId);
                let joystick = document.getElementById(draggableId);
                let isDragging = false;

                joystick.addEventListener('touchstart', function(e) {
                    e.preventDefault(); 
                    isDragging = true;
                });

                window.addEventListener('touchend', function(e) {
                    isDragging = false;
                });

                container.addEventListener('touchmove', function(e) {
                    if (!isDragging) return;

                    let bounds = container.getBoundingClientRect();
                    let x = e.touches[0].clientX - bounds.left;
                    let y = e.touches[0].clientY - bounds.top;

                    if (x < 0) x = 0;
                    if (y < 0) y = 0;
                    if (x > bounds.width) x = bounds.width;
                    if (y > bounds.height) y = bounds.height;

                    joystick.style.left = x + 'px';
                    joystick.style.top = y + 'px';

                    let servoX = Math.floor((x / bounds.width) * 180);
                    let servoY = Math.floor((y / bounds.height) * 180);

                    fetch(`/setJoystick?servoXId=${servoXId}&servoXValue=${servoX}&servoYId=${servoYId}&servoYValue=${servoY}`);
                });
            }
            setupJoystick('joystickContainer1', 'draggable1', 0, 1);
            setupJoystick('joystickContainer2', 'draggable2', 2, 3);
        </script>
    </body>
    </html>
    )html";
}

void setupWiFiAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < 4; ++i)
  {
    servos[i].attach(servosPins[i]);
  }

  setupWiFiAP();

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", getHTML()); });

  server.on("/setJoystick", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        int servoXId = request->arg("servoXId").toInt();
        int servoXValue = request->arg("servoXValue").toInt();
        int servoYId = request->arg("servoYId").toInt();
        int servoYValue = request->arg("servoYValue").toInt();

        servos[servoXId].write(servoXValue);
        servos[servoYId].write(servoYValue);

        request->send(200, "text/plain", "OK"); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/html", getHTML()); });

  server.begin();
}

void loop()
{
  dnsServer.processNextRequest();
}
