# Waveshare-Pico-LoRa-SX1262-With-Pico-2W-Json
This is a C++ program that uses a Waveshare-Pico-LoRa-SX1262 paired with a Raspberry Pi Pico 2W to send a dummy Json file.
It communicates over 686M, please check which frequency your country allows you to transmit at.
* This does not work... yet

Note:
|  Band   |  Frequency Range  |          Key Regions          |                       Characteristics                     |
|---------|-------------------|-------------------------------|-----------------------------------------------------------|
| 433 MHz | 433.05–434.79 MHz | Europe (EU433), Asia, UK      | Better penetration through walls/vegetation               |
|         |                   | Amateur radio worldwide       | Longer wavelength (~69 cm)                                |
|         |                   |                               | Often lower ERP limits (10 mW / 10% |duty cycle in EU)    |
|---------|-------------------|-------------------------------|-----------------------------------------------------------|
| 868 MHz | 863–870 MHz       | Europe (EU868)                | Higher allowed power (up to 27 dBm)                       |
|         |                   |                               | shorter range in dense environments                       |
|---------|-------------------|-------------------------------|-----------------------------------------------------------|
| 915 MHz |  902–928 MHz      | North America, Australia, NZ	| Similar to 868 MHz, higher duty cycles permitted          |
|---------|-------------------|-------------------------------|-----------------------------------------------------------|
