const { PromiseSocket, TimeoutError } = require("promise-socket")
const { Corellium } = require("@corellium/corellium-api");
const fs = require("fs");
const path = require("path");

// Helper function for sleep...
function sleep(ms) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}

// Helper function for reading sensors...
async function readSensor(ip, sensor) {
    const client = new PromiseSocket()
    await client.connect(80, ip)
    await client.writeAll('GET /Read_' + sensor + ' HTTP/1.0\r\n', 1024)
    const result = await client.readAll()
    await sleep(500)
    client.destroy()
    return Number(result.toString()).toFixed(2)
}

// Helper function for humidity...
async function readHumiditySensor(instance) {
    return await readSensor(instance.info.wifiIp, "Humidity")
}

// Helper function for pressure...
async function readPressureSensor(instance) {
    return await readSensor(instance.info.wifiIp, "Pressure")
}

// Helper function for temperature...
async function readTemperatureSensor(instance) {
    return await readSensor(instance.info.wifiIp, "Temperature")
}

// Main...
async function main() {

    // Instantiate some vars...
    let wifi_sleep = 5000;
    let fw = "STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/STM32CubeIDE/Debug/IOT_HTTP_WebServer.elf";
    let instance_name = "rumpf-stm32u5";
    let instance_flavor = "stm32u5-b-u585i-iot02a";

    // Login....
    console.log("\nCI/CD test run starting...");
    let corellium = new Corellium({
        endpoint: "https://arm.corellium.io",
        username: "christopher.rumpf@arm.com",
        password: "hufjus-6hanfy-rasxUh",
    });
    console.log("Logging in...");
    await corellium.login();

    // Setup the projects data structure...
    console.log("Getting projects list...");
    let projects = await corellium.projects();
    let project = projects[0];

    // Setup the instances data structure...
    console.log("Getting " + instance_name + ", " + instance_flavor + " instance...");
    let instances = await project.instances();
    let stm_instance = instances.find((instance) => instance.name === instance_name);

    // Create the instance if it does not already exist...
    if (!stm_instance) {
        console.log('[!] device not found, creating...');
        let customFirmware = await project.uploadIotFirmware(fw, path.basename(fw), (progress) => { })
        stm_instance = await project.createInstance({
            name: instance_name,
            flavor: instance_flavor,
            os: '1.0.0',
            bootOptions: { kernel: customFirmware }
        })
        await stm_instance.waitForState('on')
    // Reuse existing instance if it already exists...
    } else {    
        let customFirmware = await project.uploadIotFirmware(fw, path.basename(fw), (progress) => { })
        if (!customFirmware) {
            console.log('[!] Upload failed');
            return
        } else {
            console.log('[+] Upload Successful...');
        }

        if (stm_instance.state === 'off') {
            console.log('[+] Starting...');
            await stm_instance.start();
        } else {
            console.log('[+] Rebooting...');
            await stm_instance.reboot();
        }
    }

    // Sleep to let wifi connect...
    console.log("Script sleeping " + wifi_sleep + "ms to allow wifi to connect...");
    await sleep(wifi_sleep);

    // Set sensor ranges...
    var temp_low = 20;
    var temp_high = 30;
    var press_low = 980;
    var press_high = 1030;
    var humid_low = 20;
    var humid_high = 70;

    // Set initial sensor values...
    await stm_instance.modifyPeripherals({
        "temperature": "25.0",
        "pressure": "1005.0",
        "humidity": "45.0"
    });

    // Run 10 test cases...
    var x = 0.0;
    for (let i = 0; i < 10; i++) {

        console.log("\nTest run " + i + "...");
        // Generate sensor values for this test case...
        let temp_cur = (Math.round((25 + Math.sin(x) * ((temp_high - temp_low) / 2)) * 4) * 0.25).toFixed(2);
        let press_cur = (1005 + Math.sin(x) * ((press_high - press_low) / 2)).toFixed(2);
        let humid_cur = (45 + Math.sin(x) * ((humid_high - humid_low) / 2)).toFixed(2);

        // Set sensor values for this test case...
        console.log("Setting sensor values : [*] T: %f, P: %f, H: %f", temp_cur, press_cur, humid_cur);
        await stm_instance.modifyPeripherals({
            temperature: temp_cur.toString(),
            pressure: press_cur.toString(),
            humidity: humid_cur.toString(),
        });

        // Read sensor values for this test case...
        let peripherals = await stm_instance.getPeripherals();
        const temp_result = Number(peripherals.temperature).toFixed(2);
        const press_result = Number(peripherals.pressure).toFixed(2);
        const humid_result = Number(peripherals.humidity).toFixed(2);
        console.log("Getting sensor values : [*] T: %f, P: %f, H: %f", temp_result, press_result, humid_result);

        // Ensure temperature correctness...
        if (temp_result.toString() !== temp_cur.toString()) {
            console.log("Temperature Sensor returned bad value:  " + temp_result.toString() + " cur: " + temp_cur.toString());
            return;
        }

        // Ensure pressure correctness...
        if (press_result.toString() !== press_cur.toString()) {
            console.log("Pressure Sensor returned bad value: " + press_result.toString() + " cur: " + press_cur.toString());
            return;
        }

        // Ensure humidity correctness...
        if (humid_result.toString() !== humid_cur.toString()) {
            console.log("Humidity Sensor returned bad value: " + humid_result.toString());
            return;
        }

        // Randomization...
        x += Math.PI / 20.0;
    }

    // Test complete, cleanup...
    console.log('\n[+] Test run completed, destroying ' + instance_name + '...\n');
    await stm_instance.destroy();

}

// Exception handling...
main().catch((err) => {
    console.error(err);
});