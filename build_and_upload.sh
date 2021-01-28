#!/bin/bash
arduino-cli compile --fqbn arduino:avr:diecimila src/henio2/ -p /dev/ttyUSB0 --upload
