#!/usr/bin/env python3
import time
import sys

# Print HTTP headers first
print("Content-type:text/html\r\n\r\n")
print("<html><head><title>Endless Output</title></head><body>")
print("<h1>Starting endless output...</h1>")

# Continuous output to stdout
counter = 0
while True:
    counter += 1
    print(f"<p>Output line #{counter} - {time.strftime('%H:%M:%S')}</p>")
    sys.stdout.flush()  # Force flush the output buffer
