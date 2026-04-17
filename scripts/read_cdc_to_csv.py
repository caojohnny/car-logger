#!/usr/bin/env python3
"""
read_cdc_to_csv.py

Reads raw bytes from a USB CDC device (e.g. /dev/ttyACM0), finds packets between
"START>" and "<END", treats the payload as repeating packed CollectedData structs
(from an STM32 little-endian), decodes fields, applies OBD-II transfer function
for 16-bit OBD values ((256*A + B)/4), and writes a CSV.

Struct (packed, little-endian):
uint32_t time;           -> 4 bytes (ms)
uint16_t engineSpeed;    -> 2 bytes (A=MSB, B=LSB in transfer function)
uint8_t vehicleSpeed;    -> 1 byte
uint8_t throttlePosition;-> 1 byte
uint8_t tankLevel;       -> 1 byte
uint16_t fuelRate;       -> 2 bytes

Total size = 11 bytes per struct
"""

import serial
import csv
import struct
import argparse
import time
from datetime import datetime

# Constants
START_MARKER = b"START>"
END_MARKER = b"<END"
STRUCT_FORMAT = "<I H B B B H"   # little-endian: uint32, uint16, uint8, uint8, uint8, uint16
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)  # should be 11

def obd16_to_value(u16):
    # u16 is a 16-bit integer where A = MSB, B = LSB and transfer is (256*A + B)/4
    # That's equivalent to u16 / 4 if u16 stores (A<<8)|B
    return u16

def parse_payload(payload):
    """
    payload: bytes that is expected to be a sequence of STRUCT_SIZE-length packed structs.
    Returns list of dicts with decoded and converted values.
    Ignores any trailing incomplete chunk.
    """
    records = []
    n_full = len(payload) // STRUCT_SIZE
    for i in range(n_full):
        chunk = payload[i*STRUCT_SIZE:(i+1)*STRUCT_SIZE]
        # Unpack according to little-endian STM32 memory layout
        time_ms, engine_u16, vehicle_u8, throttle_u8, tank_u8, fuel_u16 = struct.unpack(STRUCT_FORMAT, chunk)
        record = {
            "time_ms": time_ms,
            "engineSpeed_rpm": obd16_to_value(engine_u16) / 4.0,
            "vehicleSpeed_kph": vehicle_u8,
            "throttlePosition_pct": throttle_u8 * (100.0 / 255.0),
            "tankLevel_pct": tank_u8 * (100.0 / 255.0),
            "fuelRate_lph_equiv": obd16_to_value(fuel_u16) / 20.0
        }
        records.append(record)
    return records

def write_csv(filename, records, append=False):
    fieldnames = ["time_ms","engineSpeed_rpm","vehicleSpeed_kph","throttlePosition_pct","tankLevel_pct","fuelRate_lph_equiv"]
    mode = "a" if append else "w"
    with open(filename, mode, newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not append:
            writer.writeheader()
        for r in records:
            writer.writerow(r)

def main():
    parser = argparse.ArgumentParser(description="Read CDC device and save collected data to CSV.")
    parser.add_argument("--device", "-d", default="/dev/ttyACM0", help="Serial device (default /dev/ttyACM0)")
    parser.add_argument("--baud", "-b", type=int, default=1000000, help="Baud rate (default 1000000)")
    parser.add_argument("--outfile", "-o", default="collected_data.csv", help="CSV output filename")
    parser.add_argument("--timeout", "-t", type=float, default=1.0, help="Serial read timeout (s)")
    parser.add_argument("--run-time", "-r", type=float, default=0.0, help="Run for N seconds then exit (0 = run forever)")
    args = parser.parse_args()

    ser = serial.Serial(args.device, args.baud, timeout=args.timeout)
    print(f"Opened {args.device} @ {args.baud}, struct size {STRUCT_SIZE} bytes")

    ser.write(b"download\r")
    ser.flush()

    buffer = bytearray()
    start_time = time.time()
    try:
        while True:
            # Optionally stop after run-time seconds
            if args.run_time > 0 and (time.time() - start_time) >= args.run_time:
                print("Run time reached, exiting loop.")
                break

            chunk = ser.read(2048)
            if not chunk:
                continue
            buffer.extend(chunk)
            print(f"Buffering serial input (buffer len = {len(buffer)})")

            # Find start marker
            s_idx = buffer.find(START_MARKER)
            if s_idx == -1:
                raise AssertionError(f"Failed to find the START_MARKER - {buffer}")

            # Find end marker after start
            e_idx = buffer.find(END_MARKER, s_idx + len(START_MARKER))
            if e_idx == -1:
                # wait for more data
                # optionally cut buffer to start at s_idx to avoid memory growth
                if s_idx > 0:
                    buffer = buffer[s_idx:]
                continue

            # Extract payload between markers
            payload_start = s_idx + len(START_MARKER)
            payload = bytes(buffer[payload_start:e_idx])

            # Remove processed region from buffer
            buffer = buffer[e_idx + len(END_MARKER):]

            # Parse payload into records
            print("Found all valid data. Parsing payload.")
            records = parse_payload(payload)
            if not records:
                print("No complete records in payload (maybe incomplete), skipping.")
                continue

            # Append to CSV
            append = True  # always append after first write; write_csv will create header only if append==False
            # If file doesn't exist yet, create with header
            try:
                with open(args.outfile, "r"):
                    pass
            except FileNotFoundError:
                append = False

            write_csv(args.outfile, records, append=append)
            print(f"Wrote {len(records)} records to {args.outfile} (payload {len(payload)} bytes)")
            break
    except KeyboardInterrupt:
        print("Interrupted by user.")
    finally:
        ser.close()
        print("Serial port closed.")

if __name__ == "__main__":
    main()

