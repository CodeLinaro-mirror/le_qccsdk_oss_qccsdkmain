#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
## MQTT Demo

## Config MQTT Demo
1.Config transport layer mode of MQTT in "prj.conf".
If choose MQTT over TCP mode, set CONFIG_MQTT_PLAINTEXT_DEMO=y;
If choose MQTT over SSL one way authentication mode, set CONFIG_MQTT_BASIC_TLS_DEMO=y;
If choose MQTT over SSL mutual way authentication mode, set CONFIG_MQTT_MUTUAL_AUTH_DEMO=y;
Notes: Only one of the above three modes can be set to “y” at a time.

2.Config broker parameter of MQTT in "demo_config.h"
If CONFIG_MQTT_PLAINTEXT_DEMO=y, two parameters must be set in mqtt_demo_plaintext/demo_config.h:
BROKER_ENDPOINT, which indicates the IP address of the broker.
BROKER_PORT, which indicates the MQTT listening port number of broker over TCP.

If CONFIG_MQTT_BASIC_TLS_DEMO=y, three parameters must be set in mqtt_demo_basic_tls/demo_config.h:
BROKER_ENDPOINT, which indicates the IP address of the broker.
BROKER_PORT, which indicates the MQTT listening port number of broker over SSL.
ROOT_CA_CERT_PATH, which indicates the path of the broker's root CA certificate.

If CONFIG_MQTT_MUTUAL_AUTH_DEMO=y, five parameters must be set in mqtt_demo_mutual_auth/demo_config.h:
BROKER_ENDPOINT, which indicates the IP address of the broker.
BROKER_PORT, which indicates the MQTT listening port number of broker over SSL.
ROOT_CA_CERT_PATH, which indicates the path of the broker's root CA certificate.
CLIENT_CERT_PATH, which indicates the path of the client certificate.
CLIENT_PRIVATE_KEY_PATH, which indicates the path of the private key.

3.Config AP SSID in "mqtt_demo.h"

## Running the demos
If the above parameters are configured, firstly compile the mqtt_demo, then flash the image(if CONFIG_MQTT_BASIC_TLS_DEMO=y or CONFIG_MQTT_MUTUAL_AUTH_DEMO=y) and filesystem to the embedded device.

For example 1:
If set CONFIG_MQTT_PLAINTEXT_DEMO = y, run the following steps:
1.Compile the mqtt_demo using build.py file in the fermion root directory.
"python build.py -i FERMION_MQTT_DEMO -b qcc730v2_evb11_hostless -o output/qcc730v2_evb11_hostless"
2.Flash the filesystem using nvm_programmer.py in ./tools/nvm_programmer.
"python nvm_programmer.py -s ch347  -f ..\..\output\qcc730v2_evb11_hostless\FERMION_MQTT_DEMO\DEBUG\bin\FERMION_MQTT_DEMO_HASHED.elf -P -A --reset"

For example 2:
If set CONFIG_MQTT_BASIC_TLS_DEMO = y or CONFIG_MQTT_MUTUAL_AUTH_DEMO = y， the SSL certificate needs to be generated and flashed to the embedded device first.

1.Generating certificates 
Reference link: https://mosquitto.org/man/mosquitto-tls-7.html.
2.flash the image to the embedded device.
2.1.Install littlefs-tools for python. Run in command line. 
pip install littlefs-tools 
2.2.Put the "ca.crt"、"client.crt"、"client.key" to ./tools/fs_tools/mycert.
2.3.Generate lfs image in ./tools/fs_tools.
python lfsimg.py -s mycert\
2.4.Parse lfsimage to check if the lfsimage generate correctly.
python lfsimg.py -f lfsimg.bin 

if the lfsimage generate correctly, the following is the result of the execution of the preceding code
lfsimg.bin
  *---ca.crt (1.4 KiB)
  *---client.crt (1.3 KiB)
  *---client.key (1.7 KiB)
2.5.Download fermion image and then download lfs image at flash 0x3000 offset
python nvm_programmer.py -s ch347  -f  ..\fs_tools\lfsimg.bin -b 0x3000 -n flash --reset
3.Compile the mqtt_demo using build.py file in the fermion root directory.
"python build.py -i FERMION_MQTT_DEMO -b qcc730v2_evb11_hostless -o output/qcc730v2_evb11_hostless"
4.Flash the filesystem using nvm_programmer.py in ./tools/nvm_programmer.
"python nvm_programmer.py -s ch347  -f ..\..\output\qcc730v2_evb11_hostless\FERMION_MQTT_DEMO\DEBUG\bin\FERMION_MQTT_DEMO_HASHED.elf -P -A --reset"
