#!/bin/bash

rsync -av esp8266_spindle_sk.esp8266.esp8266.generic.bin ElecRpi:~/BURN/.
ssh ElecRpi "cd BURN/; ./burn2esp8266.sh"
