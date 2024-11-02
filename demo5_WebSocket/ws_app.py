#!/usr/bin/env python

import asyncio
import websockets

connected = set()

async def send_chat(websocket):
    """
    Receive and process chat messages from a user.
    """
    async for message in websocket:
        # Broadcast message to all connected websockets except the sender
        for ws in connected:
            if ws != websocket:
                await ws.send(message)

async def start(websocket):
    """
    Handle a connection: start a new chat.
    """
    print("one connected")
    connected.add(websocket)

    try:
        await send_chat(websocket)
    except RuntimeError as exc:
        print(exc)
    finally:
        # Remove websocket from connected set when connection is closed
        connected.remove(websocket)

async def handler(websocket):
    """
    Handle a connection and dispatch it according to who is connecting
    (either join a chatroom or start one).
    """
    await start(websocket)

async def main():
    """
    Start a WebSockets server.
    """
    async with websockets.serve(handler, "", 8001):  # Removed SSL context
        await asyncio.Future()  # Run forever

# Entry point
if __name__ == "__main__":
    asyncio.run(main())
