# YARC - Yet Another Reflow Controller

### Setup Instructions

1. Install plugin platformio
2. Create project for your board
3. To match your wiring set the defines the corresponding value in _pindefines.h_
5. TFT_eSPI -> User_Setup.h
6. lvgl -> lv_conf.h setup from template
7. Change the include path in lv_liv_qrcode from "lvgl/lvgl.h" to "lvgl.h"

### Unit Testing

To run unit tests execute `platformio test....` in command line
