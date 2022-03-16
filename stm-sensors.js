/**
* run with: node stm-sensors.js
*/

const {Corellium} = require('../src/corellium');
const split2 = require("split2");
const path = require("path");
const { PromiseSocket, TimeoutError } = require("promise-socket")

function sleep(ms) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

async function readSensor(ip, sensor) {
  const client = new PromiseSocket()
  await client.connect(80, ip)
  await client.writeAll('GET /Read_' + sensor + ' HTTP/1.0\r\n', 1024)
  const result = await client.readAll()
  await sleep(500)
  client.destroy()
  return Number(result.toString()).toFixed(2)
}

async function readHumiditySensor(instance) {
  return await readSensor(instance.info.wifiIp, "Humidity")
}

async function readPressureSensor(instance) {
  return await readSensor(instance.info.wifiIp, "Pressure")
}

async function readTemperatureSensor(instance) {
  return await readSensor(instance.info.wifiIp, "Temperature")
}

async function waitforWifi(instance) {
  const consoleStream = await instance.console();
  const testTimeout = setTimeout( async () => {
    console.log("Times up!");
    await consoleStream.destroy();
  }, 50000);
  consoleStream.read();
  let isWaitingForSetup, isWaitingForConnection = false;
  let searchString = "*** End of wifi scan";
  return new Promise(async (resolve, reject) => {
    consoleStream.pipe(split2())
    .on('data', async function (line) {
      console.log('data', line);
      if (!isWaitingForConnection) {
        isWaitingForConnection = line.includes("- Network Interface connected:");
        if (isWaitingForConnection) {
          console.log("Wifi connection established")
          isWaitingForSetup = true
          await consoleStream.destroy();
          clearTimeout(testTimeout);
        }
      }
      if (!isWaitingForSetup) {
        isWaitingForSetup = line.includes("*** Please enter your wifi ssid");
        if (isWaitingForSetup) {
          // Write the SSID
          await consoleStream.write("Corellium\r\n")
          // It requires pauses between input
          await sleep(1000)
          await consoleStream.write("Corellium\r\n")
          await sleep(1000)
        }
      }
    })
    .on("error", async function (err) {
      console.log('error', err);
      reject(err);
    })
    .on("end", async function () {
      resolve(isWaitingForConnection);
    });
  });
}

async function main() {
  // Configure the API.
  let corellium = new Corellium({
    endpoint: "https://arm.corellium.io",
    username: "youremail@arm.com",
    password: "yourpass",
  });

  // Login.
  console.log('[+] Logging in...');
  await corellium.login();

  // Get the list of projects.
  console.log('[+] Getting projects list...');
  let projects = await corellium.projects();

  // Find the project called "Default Project".
  let project = projects.find(project => project.name === "Default Project");

  if (!project) {
    console.log('[!] Project not found');
    return;
  }

  // Get the instances in the project.
  console.log('[+] Getting instances...');
  let instances = await project.instances();

  console.log('[+] Getting STM32U5...');
  // find STM32U5 instance
  let instance = instances.find(instance => instance.name === 'STM32U5-test');

  let firmware = "IOT_HTTP_WebServer.elf"
  if (!instance) {
    console.log('[!] STM32U5 device not found. Creating a new one');
    let customFirmware = await project.uploadIotFirmware(firmware, path.basename(firmware), (progress) => {})
    // create the STM32U5 device
    instance = await project.createInstance({
      name: "STM32U5-test",
      flavor: 'stm32u5-b-u585i-iot02a',
      os: '1.0.0',
      bootOptions: {
        kernel: customFirmware
      }
    })
    await instance.waitForState('on')
  } else {
    // Device already exists so upload a new firmware
    console.log('[+] Found device. Uploading new firmware...');
    let customFirmware = await project.uploadIotFirmware(firmware, path.basename(firmware), (progress) => {})
    if(!customFirmware  ) {
      console.log('[!] Upload failed');
      return
    } else {
      console.log('[+] Upload Successful rebooting...');
    }

    if(instance.state === 'off') {
      await instance.start();
    } else {
      await instance.reboot();
    }
 }

  // Connect to Wifi
  await waitforWifi(instance);

  // set sensor ranges
  var temp_low = 20;
  var temp_high = 30;
  var press_low = 980;
  var press_high = 1030;
  var humid_low = 20;
  var humid_high = 70;

  // set initial values
  await instance.modifyPeripherals({
    "temperature": "25.0",
    "pressure": "1005.0",
    "humidity": "45.0"
  });


  var x = 0.0;
  // Run the test a few times
  for (let i = 0; i < 2; i++) {
    // generate sensor values
    let temp_cur = (Math.round((25 + Math.sin(x) * ((temp_high - temp_low)/2)) * 4) * 0.25).toFixed(2)
    let press_cur = (1005 + (Math.sin(x) * ((press_high - press_low)/2))).toFixed(2)
    let humid_cur = (45 + (Math.sin(x) * ((humid_high - humid_low)/2))).toFixed(2)
    console.log("Setting sensor values : [*] T: %f, P: %f, H: %f", temp_cur, press_cur, humid_cur);
    await instance.modifyPeripherals({
      "temperature": temp_cur.toString(),
      "pressure": press_cur.toString(),
      "humidity": humid_cur.toString()
    });

    // Compare the sensor levels reported by the device
    const temp_result  = await readTemperatureSensor(instance)
    const press_result = await readPressureSensor(instance)
    const humid_result = await readHumiditySensor(instance)

    console.log("Getting sensor values : [*] T: %f, P: %f, H: %f", temp_result, press_result, humid_result);

    if(temp_result.toString() !== temp_cur.toString())
    {
      console.log("Temperature Sensor returned bad value:  " + temp_result.toString() + " Set value: " + temp_cur.toString())
      return
    }

    if(press_result.toString() !== press_cur.toString())
    {
      console.log("Pressure Sensor returned bad value: " + press_result.toString() + " Set value: " + press_cur.toString())
      return
    }

    if(humid_result.toString() !== humid_cur.toString())
    {
      console.log("Humidity Sensor returned bad value: " + humid_result.toString() + " Set value:" + humid_cur.toString())
      return
    }

    x += Math.PI/20.0;
  }

  console.log("[+] done")
  return;
}

main().catch(err => {
  console.error(err);
});
