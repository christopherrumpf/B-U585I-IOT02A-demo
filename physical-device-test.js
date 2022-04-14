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

async function readHumiditySensor(ip) {
  return await readSensor(ip, "Humidity")
}

async function readPressureSensor(ip) {
  return await readSensor(ip, "Pressure")
}

async function readTemperatureSensor(ip) {
  return await readSensor(ip, "Temperature")
}


async function main() {

  ip = "10.42.0.220"

  // Compare the sensor levels reported by the device
  const temp_result  = await readTemperatureSensor(ip)
  const press_result = await readPressureSensor(ip)
  const humid_result = await readHumiditySensor(ip)

  console.log("Got sensor values : [*] T: %f, P: %f, H: %f from ", temp_result, press_result, humid_result, ip);

  return;
}

main().catch(err => {
  console.error(err);
});
