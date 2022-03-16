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
    let pdata =
    {
        "acceleration": "0.000000,9.810000,0.000000",
        "gyroscope": "0.000000,0.000000,0.000000",
        "magnetic": "0.000000,45.000000,0.000000",
        "orientation": "0.000000,0.000000,0.000000",
        "temperature": "70.000000",
        "proximity": "50.000000",
        "light": "20.000000",
        "pressure": "1013.250000",
        "humidity": "55.000000"
        };

        let pdata_orig = pdata;

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

    // Modify peripheral data...
    let peripherals = await stm_instance.getPeripherals();
    console.log(peripherals);

    for( let i=0; i<30; i++) {
        pdata.temperature++;
        pdata.proximity++;
        pdata.light++;
        pdata.pressure++;
        pdata.humidity++;
        console.log("setting peripheral data to:");
        console.log(pdata);
        await stm_instance.modifyPeripherals(pdata);

        if ( i == 20 ) {
            pdata = pdata_orig;
        }
    }

    //    console.log("Getting rumpf-pi instance...");
    //    let pi_instance = instances.find((instance) => instance.name === "rumpf-pi");

    return;
}

main().catch((err) => {
    console.error(err);
});
