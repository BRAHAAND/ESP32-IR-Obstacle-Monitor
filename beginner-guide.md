# Beginner's Guide: Build the ESP32 IR Obstacle Monitor

This guide takes you from an unopened ESP32 to a live web dashboard that shows whether something is in front of an infrared sensor. No electronics or programming experience is needed. Go one step at a time and use the checkpoint at the end of each step to make sure it worked before moving on.

**Time needed:** about 1 to 1.5 hours.

## What you will build

An ESP32 board reads an infrared (IR) obstacle sensor and hosts a small website. You open the website on your phone or laptop, and it shows in real time whether an obstacle is present. A switch on the page lets you turn the sensor on or off.

## What you need

| Item | Notes |
|---|---|
| ESP32 development board | The common 30-pin DevKit is used in this guide |
| FC-51 IR obstacle sensor module | Has 3 pins: VCC, GND, OUT |
| 3 jumper wires | Female-to-male for direct connection, or male-to-male with a breadboard |
| Breadboard | Optional |
| USB cable | Must carry **data**, not just charge |
| A computer | Windows, Mac or Linux |
| A 2.4 GHz Wi-Fi network | The ESP32 cannot connect to 5 GHz-only networks |

## A few ideas explained simply

- **ESP32:** a small computer on a chip with built-in Wi-Fi. You write a program (a "sketch"), upload it, and it runs by itself.
- **GPIO pin:** a pin on the board your program can read or control. "GPIO21" is pin number 21.
- **IP address:** the address of a device on your Wi-Fi network, for example `192.168.1.45`. You type it into a browser to reach the ESP32.
- **HTTP server:** a program that answers browser requests with web pages. Here, the ESP32 is the server.
- **Serial Monitor:** a window in the Arduino IDE that shows messages the ESP32 prints through the USB cable.

## Step 1: Install the Arduino IDE

1. Download the Arduino IDE from [arduino.cc/en/software](https://www.arduino.cc/en/software) and install it.
2. Open it once to make sure it starts.

**Checkpoint:** the Arduino IDE opens with a blank sketch.

## Step 2: Add ESP32 support

1. Open **File → Preferences** (on Mac: Arduino IDE → Settings).
2. In "Additional boards manager URLs", paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click OK.
4. Open **Tools → Board → Boards Manager**, search for **esp32**, and install **esp32 by Espressif Systems** (version 3.x). This takes a few minutes.

**Checkpoint:** after installing, **Tools → Board** shows an "esp32" section.

## Step 3: Connect the board and test the upload

1. Plug the ESP32 into your computer with the USB cable.
2. Select **Tools → Board → esp32 → ESP32 Dev Module**.
3. Select the correct port in **Tools → Port**. If no port appears, try another cable (many are charge-only) or install the USB driver for your board (**CP210x** or **CH340**).
4. Open **File → Examples → 01.Basics → Blink** and click **Upload** (the right arrow).
5. If the upload hangs at `Connecting...`, hold the **BOOT** button on the board until it starts.

**Checkpoint:** the onboard LED blinks. If it does, your computer and board can talk to each other.

## Step 4: Wire the sensor

Turn the ESP32 off (unplug USB) before wiring.

| FC-51 pin | ESP32 pin |
|---|---|
| VCC | **3V3** |
| GND | GND |
| OUT | **GPIO21** (labelled D21) |

<img width="2048" height="1440" alt="image" src="https://github.com/user-attachments/assets/3811509c-5a5c-4035-8d7d-8284b0f35557" />



Important points:

- **Use 3V3, not 5V or VIN.** The ESP32's pins can be damaged by 5V signals.
- **Read the labels on your sensor.** Many are printed in the order **OUT, GND, VCC**, which is the reverse of what you might expect.
- On a 30-pin DevKit with the USB port at the bottom, D21, GND and 3V3 are all on the right-hand row of pins. Pin positions differ between boards, so match the printed labels.

**Checkpoint:** the sensor's red power LED lights up when you plug the USB cable back in. If it does not, re-check VCC and GND and press the wires in firmly.

## Step 5: Test the sensor on its own

Before adding Wi-Fi, check that the sensor works. Create a new sketch, paste this, and upload it:

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(21, INPUT);
}

void loop() {
  int detected = (digitalRead(21) == LOW) ? 1 : 0;  // LOW = obstacle seen
  Serial.println(detected);
  delay(100);
}
```

1. Open **Tools → Serial Monitor** and set the baud rate (bottom right) to **115200**.
2. Hold your hand or a sheet of white paper about 10 cm in front of the sensor, then pull it away.

**Checkpoint:** the monitor prints `0` normally and `1` while your hand is near, and the sensor's second LED lights at the same time.

If it does not behave this way:

| What you see | What to do |
|---|---|
| Only `1`s | Turn the blue screw (potentiometer) counter-clockwise until the second LED switches off. Point the sensor at open space. Check that OUT is in the GPIO21 pin. |
| Only `0`s | Turn the screw clockwise slowly while holding white paper 5 cm away, until the LED lights. |
| The LED flickers | The object is right at the detection limit. Move it closer or adjust the screw until the LED is steady. |
| Nothing prints | Set the baud rate to 115200, check the port, and press the **EN** button on the board. |

## Step 6: Load the project code

1. Create a new sketch in the Arduino IDE and save it as `esp32-ir-obstacle-monitor`.
2. Delete everything in it and paste in the code from `firmware/esp32-ir-obstacle-monitor/esp32-ir-obstacle-monitor.ino` in this repository.
3. Near the top of the code, enter your Wi-Fi details between the quotes:
   ```cpp
   const char* ssid     = "YOUR_WIFI_NETWORK_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
   Spelling and capital letters must match exactly.

> If you share your code online later, put the placeholders back first so your real password is not published.

## Step 7: Upload and find the address

1. Click **Upload**. Hold **BOOT** if it stops at `Connecting...`.
2. Open **Serial Monitor** at **115200 baud** and press the **EN** button on the board.
3. Wait for the dots to finish. You should see something like:
   ```
   Connecting to MyHomeWiFi
   ......
   WiFi connected.
   Open this address in your browser: http://192.168.1.45
   ```

**Checkpoint:** an address starting with `http://` appears. Yours will have different numbers.

If it only prints dots forever: check the name and password, make sure the network is **2.4 GHz**, and move closer to the router.

## Step 8: Open the dashboard

1. On a phone or laptop connected to the **same Wi-Fi network**, open a browser and type the address from the Serial Monitor.
2. You should see the dashboard with a green ring and "Path clear".

**Checkpoint:** the badge at the top right says **Live**.

If the page does not load:

- Make sure the device is on the same network as the ESP32 (not mobile data, a guest network or a VPN).
- Type the address exactly, starting with `http://` (not `https://`).
- Press **EN** on the board and check the Serial Monitor for the address again, since it can change after a restart.

## Step 9: Try it out

1. Wave your hand in front of the sensor. The ring turns red and pulses, "Detections" goes up, and the onboard LED lights.
2. Flip the **Sensor input** switch off. The ring turns grey and the sensor is ignored, even if you wave your hand.
3. Flip it back on and detection resumes.
4. Unplug the ESP32. After a couple of seconds the page shows "Connection lost". Plug it back in and it recovers by itself.

You have built a working IoT device.

## How the code works

| Part of the sketch | What it does |
|---|---|
| `INDEX_HTML` | The whole web page (HTML, CSS and JavaScript), stored inside the ESP32's memory |
| `updateSensor()` | Reads the sensor pin and only accepts a change after it has been stable for 40 ms (this is called debouncing) |
| `setSensorEnabled()` | Turns sensor reading on or off when you use the switch |
| `statusJson()` | Builds the small data message the page asks for every half second |
| `handleRequest()` | Looks at which address the browser asked for and replies with the page or the data |
| `setup()` | Runs once: sets up pins, joins Wi-Fi, starts the server |
| `loop()` | Runs forever: reads the sensor and answers browsers |

The flow in plain words: the browser loads the page once, then keeps asking the ESP32 "what is the sensor doing?" twice a second and redraws the page with the answer.

## Experiments to try

1. **Change the text.** Edit `Path clear` in the page to something else, upload, and refresh.
2. **Change the colours.** Find `--c:#2fd884` (the green) in the CSS and try another colour.
3. **Change the debounce.** Set `DEBOUNCE_MS` to `200` and notice the slower response.
4. **Use another pin.** Move the OUT wire to a different safe pin (for example GPIO19) and update `SENSOR_PIN`.
5. **Read the raw data.** Open `http://<your-address>/api/status` in a browser to see what the dashboard reads.

## Troubleshooting

| Problem | Likely cause and fix |
|---|---|
| No port in Tools → Port | Use a data-capable USB cable. Install the CP210x or CH340 driver. |
| Upload stops at `Connecting...` | Hold the **BOOT** button until the upload starts. |
| `WiFi.h: No such file` when compiling | Wrong board selected. Choose **ESP32 Dev Module**. |
| Serial Monitor shows strange symbols | Set the baud rate to **115200**. |
| Only dots, never connects | Wrong name or password, 5 GHz network, or weak signal. |
| Page will not load | Different network from the ESP32, VPN on, or wrong address. Re-check the Serial Monitor. |
| Always shows "Obstacle detected" | Turn the sensor's potentiometer counter-clockwise and point it at open space. |
| Never detects | Turn the potentiometer clockwise and use white paper. Dark surfaces are hard for IR sensors. |
| Module's power LED is off | Check VCC to 3V3 and GND to GND, and the pin labels on the module. |
| Page shows "Connection lost" | The ESP32 lost power or Wi-Fi. Check the board and router. |

## Glossary

- **Sketch:** an Arduino program.
- **Upload / flash:** copying your program onto the ESP32.
- **Baud rate:** the speed of the USB serial connection. Both sides must use the same value (115200).
- **Debouncing:** ignoring very short flickers in a signal so that only stable changes count.
- **API:** a set of addresses a program can request to get data or trigger actions.
- **JSON:** a simple text format for sending data, such as `{"obstacle":false}`.
- **Active LOW:** a signal that means "yes" when it goes to 0 volts. The FC-51 works this way.

## Where to go next

- Read the **[project README](../README.md)** for the design decisions and API details.
- Add an ultrasonic sensor to measure real distance.
- Add a motor driver and build the obstacle-avoiding robot this project is leading towards.
