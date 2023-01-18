# Modify the following line with your own serial port details
#   Currently set COM3 as serial port at 115.2kbps 8N1
#   Refer to PySerial API for other options.  One option to consider is
#   the "timeout" - allowing the program to proceed if after a defined
#   timeout period.  The default = 0, which means wait forever.

import csv
import serial
import math

com_port = "COM3"

coordinate_file = []

step_distance = 900
x=0
file = open(r"values.xyz", "w+", newline="")
with file:
    with serial.Serial(com_port, baudrate=115200, timeout=1) as ser:
        while ser.readable():
            ser.reset_output_buffer()
            ser.reset_input_buffer()
            line = ser.readline()  # read a '\n' terminated line

            try:
                coordinates = line.decode().strip().split(",")
                r = int(coordinates[0])
                theta = int(coordinates[1])
                z = r * math.sin(math.radians(theta))
                y = r * math.cos(math.radians(theta))
                print(x, y, z)
                coordinate_file.append([x, y, z])
            except:
                if line.decode().strip():
                    print(line.decode().strip())

            if line.decode() == "FINISH\n":
                write = csv.writer(file)
                write.writerows(coordinate_file)
                coordinate_file=[]
                x += step_distance

            if line.decode() == "ABORT\n":
                coordinate_file=[]
