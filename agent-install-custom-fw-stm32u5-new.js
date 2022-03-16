const { PromiseSocket, TimeoutError } = require("promise-socket")
const { Corellium } = require("@corellium/corellium-api");
const fs = require("fs");

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


async function main() {

    // Configure the API.
    let corellium = new Corellium({
        endpoint: "https://arm.corellium.io",
        username: "christopher.rumpf@arm.com",
        password: "hufjus-6hanfy-rasxUh",
    });

    console.log("Logging in...");
    await corellium.login();

    console.log("Getting projects list...");
    let projects = await corellium.projects();

    // Individual accounts have a default project...
    let project = projects[0];

    console.log("Getting rumpf-stm32u5 instance...");
    let instances = await project.instances();

    // Get rumpf's stm-32 instance...
    let stm_instance = instances.find((instance) => instance.name === "rumpf-stm32u5");

    // Upload firmware...
    console.log("Uploading IoT Firmware...");
    let fw_image = await stm_instance.uploadIotFirmware("STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/STM32CubeIDE/Debug/IOT_HTTP_WebServer.elf", "IOT_HTTP_WebServer.elf")

    // Reboot...
    console.log("Rebooting...");
    await stm_instance.reboot();

    // Sleep to let wifi connect
    console.log("Script sleeping 10000ms (10s) to allow wifi to connect...");
    await sleep(10000);

     // set sensor ranges
    var temp_low = 20;
    var temp_high = 30;
    var press_low = 980;
    var press_high = 1030;
    var humid_low = 20;
    var humid_high = 70;

    // set initial values
    await stm_instance.modifyPeripherals({
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
      await stm_instance.modifyPeripherals({
        "temperature": temp_cur.toString(),
        "pressure": press_cur.toString(),
        "humidity": humid_cur.toString()
      });

      // Compare the sensor levels reported by the device
      const temp_result  = await readTemperatureSensor(stm_instance)
      const press_result = await readPressureSensor(stm_instance)
      const humid_result = await readHumiditySensor(stm_instance)
  
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
  
    console.log("done")
    return;
}


main().catch((err) => {
    console.error(err);
});
