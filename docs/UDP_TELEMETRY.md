# UDP telemetry bridge (ksim)

Publish live vehicle/session state over UDP for external tools.
Complements shared-memory; does not replace it.

## Defaults

| Setting | Value |
|---------|-------|
| Host | `127.0.0.1` |
| Port | `20777` |
| Format | Binary v1 (`KSIM` magic) |
| Alt format | JSON line (`setJsonMode(true)`) |

> Port **20747** is left free for *inbound* AC-style listeners (`UdpTelemetryListener`).

## Binary packet (`UdpTelemPacket`)

```
magic[4] = 'K''S''I''M'
version  = 1
size     = sizeof(packet)
sequence uint32
timeSec  double
speedMs, rpm, throttle, brake, steer, gear, fuelL
pos/vel/accG, heading
tyreTemp/Wear/Pressure[4]
laps, sector, times ms, position, session, status
spline, grip, air/road temp, inPit, pitLimiter
```

Little-endian, packed. One datagram per sample.

## Wire-in

```cpp
m_udp = std::make_unique<UdpTelemetryBridge>();
m_udp->open("127.0.0.1", 20777);
// each tick:
m_udp->publish(sample);
```

## JSON example

```json
{"seq":12,"t":1.5,"v":120.0,"rpm":6500,"gear":4,"thr":0.9,"brk":0,"str":0.1,"fuel":40,"x":1,"y":0,"z":2,"lap":0,"sec":0,"ct":1500,"bt":0,"pos":1,"spline":0.12}
```
