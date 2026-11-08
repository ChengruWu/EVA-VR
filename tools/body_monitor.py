#!/usr/bin/env python3
"""Read-only monitor for the experimental EVA-VR Body WebSocket stream.

Requires websockets 12.x. This does not forward input to EVA or a robot.
"""
import argparse
import asyncio
import json
from urllib.parse import parse_qs, urlsplit

import websockets


async def monitor(socket, path):
    url = urlsplit(path)
    if url.path != "/ws" or parse_qs(url.query).get("token") != ["eva"]:
        await socket.close(code=1008, reason="invalid endpoint")
        return
    print("headset connected", flush=True)
    async for payload in socket:
        try:
            frame = json.loads(payload)
            body = frame.get("body")
            if frame.get("type") != "frame" or not isinstance(body, dict):
                continue
            joints = body.get("joints", [])
            if not isinstance(joints, list):
                continue
            valid = sum(1 for j in joints if isinstance(j, dict) and
                        j.get("position") is not None and j.get("orientation_xyzw") is not None)
            print(f"frame={frame.get('seq')} body={body.get('sample_sequence')} "
                  f"status={body.get('status')} valid={valid}/24 "
                  f"xr_time={body.get('xr_predicted_display_time')}", flush=True)
            if monitor.show_joints:
                print(json.dumps(body, ensure_ascii=False, allow_nan=False), flush=True)
        except (ValueError, TypeError, AttributeError):
            print("invalid input frame", flush=True)


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=43876)
    parser.add_argument("--show-joints", action="store_true")
    args = parser.parse_args()
    monitor.show_joints = args.show_joints
    async with websockets.serve(monitor, args.host, args.port, max_size=65536):
        print(f"listening on {args.host}:{args.port}; read-only, no robot output", flush=True)
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
