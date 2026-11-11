#!/usr/bin/env python3
"""Tiny UDP proxy that delays every packet by a fixed one-way latency (and can drop a share of them).

Used by tools/net_smoke_test.sh with LAG_MS=... to exercise client prediction and server reconciliation:
    udp_lag_proxy.py LISTEN_PORT SERVER_PORT ONE_WAY_MS [LOSS_PERCENT] [DURATION_S]
Clients connect to LISTEN_PORT on 127.0.0.1; the server listens on SERVER_PORT.
"""
import asyncio
import random
import sys


class ClientLink(asyncio.DatagramProtocol):
    """Socket toward the server for one client address."""

    def __init__(self, proxy, client_addr):
        self.proxy = proxy
        self.client_addr = client_addr
        self.transport = None

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        self.proxy.delay(lambda: self.proxy.transport.sendto(data, self.client_addr))


class Proxy(asyncio.DatagramProtocol):
    def __init__(self, server_port, delay_s, loss):
        self.server = ("127.0.0.1", server_port)
        self.delay_s = delay_s
        self.loss = loss
        self.links = {}
        self.transport = None

    def connection_made(self, transport):
        self.transport = transport

    def delay(self, send):
        if random.random() * 100.0 < self.loss:
            return
        asyncio.get_running_loop().call_later(self.delay_s, send)

    def datagram_received(self, data, addr):
        asyncio.ensure_future(self._forward(data, addr))

    async def _forward(self, data, addr):
        link = self.links.get(addr)
        if link is None:
            loop = asyncio.get_running_loop()
            _, link = await loop.create_datagram_endpoint(lambda: ClientLink(self, addr), remote_addr=self.server)
            self.links[addr] = link
        self.delay(lambda: link.transport.sendto(data))


async def main():
    listen_port, server_port, one_way_ms = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
    loss = float(sys.argv[4]) if len(sys.argv) > 4 else 0.0
    duration = float(sys.argv[5]) if len(sys.argv) > 5 else 60.0
    loop = asyncio.get_running_loop()
    await loop.create_datagram_endpoint(lambda: Proxy(server_port, one_way_ms / 1000.0, loss),
                                        local_addr=("127.0.0.1", listen_port))
    print(f"lag proxy :{listen_port} -> :{server_port}, {one_way_ms:.0f} ms one way, {loss:.1f}% loss", flush=True)
    await asyncio.sleep(duration)


if __name__ == "__main__":
    asyncio.run(main())
