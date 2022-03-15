################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_address.c \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_class_extension.c \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_core.c \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_os.c \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_socket.c \
/home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_state.c 

OBJS += \
./Middleware/STM32_Network_Library/core/net_address.o \
./Middleware/STM32_Network_Library/core/net_class_extension.o \
./Middleware/STM32_Network_Library/core/net_core.o \
./Middleware/STM32_Network_Library/core/net_os.o \
./Middleware/STM32_Network_Library/core/net_socket.o \
./Middleware/STM32_Network_Library/core/net_state.o 

C_DEPS += \
./Middleware/STM32_Network_Library/core/net_address.d \
./Middleware/STM32_Network_Library/core/net_class_extension.d \
./Middleware/STM32_Network_Library/core/net_core.d \
./Middleware/STM32_Network_Library/core/net_os.d \
./Middleware/STM32_Network_Library/core/net_socket.d \
./Middleware/STM32_Network_Library/core/net_state.d 


# Each subdirectory must supply rules for building sources it contributes
Middleware/STM32_Network_Library/core/net_address.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_address.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/STM32_Network_Library/core/net_class_extension.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_class_extension.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/STM32_Network_Library/core/net_core.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_core.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/STM32_Network_Library/core/net_os.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_os.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/STM32_Network_Library/core/net_socket.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_socket.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middleware/STM32_Network_Library/core/net_state.o: /home/runner/work/B-U585I-IOT02A-demo/B-U585I-IOT02A-demo/STM32CubeIDE/workspace_1.9.0/IOT_HTTP_WebServer/Middlewares/ST/STM32_Network_Library/core/net_state.c Middleware/STM32_Network_Library/core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DSTM32U585xx -DUSE_HAL_DRIVER -c -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../../Drivers/STM32U5xx_HAL_Driver/Inc -I../../Drivers/BSP/B-U585I-IOT02A -I../../Drivers/BSP/Components/mx_wifi -I../../Middlewares/ST/STM32_Network_Library/Includes -I../../Core/Inc -I../../WebServer/App -I../../WebServer/App/wifi -I../../WebServer/App/web_addons -I../../WebServer/App/sensors -I../../WebServer/App/http -I../../WebServer/Target -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middleware-2f-STM32_Network_Library-2f-core

clean-Middleware-2f-STM32_Network_Library-2f-core:
	-$(RM) ./Middleware/STM32_Network_Library/core/net_address.d ./Middleware/STM32_Network_Library/core/net_address.o ./Middleware/STM32_Network_Library/core/net_address.su ./Middleware/STM32_Network_Library/core/net_class_extension.d ./Middleware/STM32_Network_Library/core/net_class_extension.o ./Middleware/STM32_Network_Library/core/net_class_extension.su ./Middleware/STM32_Network_Library/core/net_core.d ./Middleware/STM32_Network_Library/core/net_core.o ./Middleware/STM32_Network_Library/core/net_core.su ./Middleware/STM32_Network_Library/core/net_os.d ./Middleware/STM32_Network_Library/core/net_os.o ./Middleware/STM32_Network_Library/core/net_os.su ./Middleware/STM32_Network_Library/core/net_socket.d ./Middleware/STM32_Network_Library/core/net_socket.o ./Middleware/STM32_Network_Library/core/net_socket.su ./Middleware/STM32_Network_Library/core/net_state.d ./Middleware/STM32_Network_Library/core/net_state.o ./Middleware/STM32_Network_Library/core/net_state.su

.PHONY: clean-Middleware-2f-STM32_Network_Library-2f-core

