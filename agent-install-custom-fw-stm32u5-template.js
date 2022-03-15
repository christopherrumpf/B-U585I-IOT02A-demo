const { Corellium } = require("@corellium/corellium-api");
const fs = require("fs");

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
        username: "you@arm.com",
        password: "your-password",
    });

    console.log("Logging in...");
    await corellium.login();

    console.log("Getting projects list...");
    let projects = await corellium.projects();

    // Individual accounts have a default project
    let project = projects[0];

    console.log("Getting rumpf-stm32u5 instance...");
    let instances = await project.instances();

    let stm_instance = instances.find((instance) => instance.name === "rumpf-stm32u5");

    console.log("Uploading IoT Firmware...");
    let fw_image = await stm_instance.uploadIotFirmware("Debug/IOT_HTTP_WebServer.elf", "IOT_HTTP_WebServer.elf")
    
    console.log("Rebooting...");
    await stm_instance.reboot();

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

    return;
}

main().catch((err) => {
    console.error(err);
});
