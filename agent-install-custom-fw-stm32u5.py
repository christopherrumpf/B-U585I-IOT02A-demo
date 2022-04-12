import asyncio
import io
import os
from posixpath import dirname
import re
from websockets import client as ws
from os import environ
import time
import sys

import AvhClientAsync
from AvhClientAsync.models import InstanceState
from AvhClientAsync.rest import ApiException
from pprint import pprint

import ssl
ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

if len(sys.argv) < 3:
  print('Usage: %s <ApiEndpoint> <ApiToken>', sys.argv[0])
  exit(-1)

#apiEndpoint = sys.argv[1]
#apiToken = sys.argv[2]
#fw = sys.argv[3]
apiEndpoint = "https://app.avh.arm.com/api"
apiToken = "40258cd7c66f5999fddc.302aa699501a1f9b5695c1f1a1edf4125ba62419f772303944c8b239e97e66d86aab4b3b7ad6a0ef55d334892952b46d4a261f579ba48976d2b5ed9fc743c42f"
fw = "../../STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/STM32CubeIDE/Debug/IOT_HTTP_WebServer.bin"

pprint("debug...")
pprint(apiEndpoint)
pprint(apiToken)
pprint(fw)

async def waitForState(instance, state):
  global api_instance

  instanceState = await api_instance.v1_get_instance_state(instance.id)
  while (instanceState != state):
    if (instanceState == 'error'):
      raise Exception('VM entered error state')
    await asyncio.sleep(1)
    instanceState = await api_instance.v1_get_instance_state(instance.id)

ledStates = [ 'off', 'on' ]
async def printLeds(instance):
  state = await api_instance.v1_get_instance_gpios(instance.id)
  ledBank = state['led'].banks[0]
  print('LED6: %s LED7: %s' % (ledStates[ledBank[0]], ledStates[ledBank[1]]) )

async def pressButton(instance):
  await api_instance.v1_set_instance_gpios(instance.id, {
    "button": {
      "bitCount": 1,
      "banks": [
        [1]
      ]
    }
  })
  await api_instance.v1_set_instance_gpios(instance.id, {
    "button": {
      "bitCount": 1,
      "banks": [
        [0]
      ]
    }
  })

async def testBspImage(instance):
  global api_instance
  global ctx
  text = ''
  done = False

  consoleEndpoint = await api_instance.v1_get_instance_console(instance.id)
  console = await ws.connect(consoleEndpoint.url, ssl=ctx)
  try:
    async for message in console:
      if done:
        break

      text += message.decode('utf-8')
      while '\n' in text:
        offset = text.find('\n')
        line, text = text[:offset], text[offset+1:]
        print('<< %s' % line)

        match = re.match(r'(?:Switch \S+ LED(\d)|Please press.*User (button)|\**RANGING (SENSOR)\**)', line)
        if (match):
          if match[1]:
            await printLeds(instance)
          elif match[2]:
            await pressButton(instance)
          elif match[3]:
            # Done testing GPIOs
            print('Test completed')
            done = True
            break

  finally:
    console.close_timeout = 1
    await console.close()


# Defining the host is optional and defaults to https://app.avh.arm.com/api
# See configuration.py for a list of all supported configuration parameters.

exitStatus = 0

async def main():
  global exitStatus
  global api_instance

  configuration = AvhClientAsync.Configuration(
      host = apiEndpoint
  )
  # Enter a context with an instance of the API client
  async with AvhClientAsync.ApiClient(configuration=configuration) as api_client:
    # Create an instance of the API class
    api_instance = AvhClientAsync.ArmApi(api_client)

    # Log In
    token_response = await api_instance.v1_auth_login({
      "apiToken": apiToken,
    })

    print('Logged in')
    configuration.access_token = token_response.token

    print('Finding a project...')
    api_response = await api_instance.v1_get_projects()
    pprint(api_response)
    projectId = api_response[0].id

    print('Getting our model...')
    api_response = await api_instance.v1_get_models()
    chosenModel = None
    for model in api_response:
      if model.flavor.startswith('stm32u5'):
        chosenModel = model
        break

    pprint(chosenModel)

    print('Finding software for our model...')
    api_response = await api_instance.v1_get_model_software(model.model)
    version = api_response[0].version
  
    print('Creating a new instance...')
    api_response = await api_instance.v1_create_instance({
      "name": 'STM32U5-BSP-Test',
      "project": projectId,
      "flavor": chosenModel.flavor,
      "os": version
    })
    instance = api_response

    error = None
    try:
      print('Waiting for VM to create...')
      await waitForState(instance, 'on')

      print('Setting the VM to use the bsp test software')
      api_response = await api_instance.v1_create_image('iotfirmware', 'plain', 
        name=fw,
        instance=instance.id,
        file=os.path.join(sys.path[0], fw)
      )
      pprint(api_response)

      print('Resetting VM to use the new software')
      api_response = await api_instance.v1_reboot_instance(instance.id)
      print('Waiting for VM to finish resetting...')
      await waitForState(instance, 'on')
      await asyncio.sleep(10)
      print('done')

    except Exception as e:      
      error = e

    if error != None:
      raise error

asyncio.run(asyncio.wait_for(main(), 120))
exit(0)