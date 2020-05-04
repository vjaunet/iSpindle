#
#      Simple MAkefile based on Arduino-cli
#
# =============================================

FBQN=esp8266:esp8266:generic
SKETCH=../esp8266_spindle_sk

ISP_PORT=$(shell ls /dev/cu.wchusbserial14??)

ALL:
	arduino-cli compile --fqbn ${FBQN} ${SKETCH}

upload:
	arduino-cli upload -p ${ISP_PORT} --fqbn ${FBQN} ${SKETCH}

clean:
	rm -rf *.bin *~ *.elf
