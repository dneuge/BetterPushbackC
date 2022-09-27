#!/bin/env python3

# CDDL HEADER START
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#
# CDDL HEADER END

import json
import sys

from PIL import Image, ImageDraw

if len(sys.argv) != 4:
	print('Call with 1. path to JSON file, 2. path to output file, 3. scale factor', file=sys.stderr)
	sys.exit(1)

dump_path = sys.argv[1]
out_path = sys.argv[2]
scale = float(sys.argv[3])

with open(dump_path, 'r') as fh:
	dump = json.load(fh)

acf_filepath = dump['file']
print('Outline dump was created from '+acf_filepath, file=sys.stderr)

log = dump['log']
if len(log) > 0:
	print('The following messages were logged during outline parsing:', file=sys.stderr)
	for line in log:
		print('  '+line, file=sys.stderr)

if not dump['success']:
	print('The aircraft file failed parsing, nothing to plot.', file=sys.stderr)
	sys.exit(1)

max_abs_x = 0.0
max_abs_y = 0.0

objects = {
	'fuselage': {
		'color': 'black'
	},
	'main_wing': {
		'color': 'blue'
	},
	'stab_wing': {
		'color': 'red'
	}
}

for key in objects.keys():
	for coord in dump[key]:
		max_abs_x = max(abs(coord[0]), max_abs_x)
		max_abs_y = max(abs(coord[1]), max_abs_y)

image_width = round(max_abs_x * scale * 2.0) + 4
image_height = round(max_abs_y * scale * 2.0) + 4

image = Image.new('RGB', (image_width, image_height), color='white')

for mirror_x in [-1.0, 1.0]:
	for key, config in objects.items():
		draw = ImageDraw.Draw(image)
		prev_pos = None
		for coord in dump[key]:
			pos = (mirror_x * coord[0] * scale + (image_width/2), coord[1] * scale + (image_height/2))
			
			if prev_pos is not None:
				draw.line([prev_pos, pos], fill=config['color'], width=1)
			
			prev_pos = pos

image.save(out_path)
