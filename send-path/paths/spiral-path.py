#!/usr/bin/env python3

from math import sin, cos

points = 300

cx = 0xff80
cy = 0xff00

for i in range(0,points):
	ang = i * 10.0 / points
	r = 20.0 + i * 30.0 / points
	x = int(round(r*cos(ang)+cx))
	y = int(round(r*sin(ang)+cy))
	print(str(x) + " " + str(y))
