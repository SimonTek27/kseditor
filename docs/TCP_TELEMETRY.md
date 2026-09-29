# Comunicazione TCP in ksim — esplorazione

## Stato repo (oggi)

| Canale | Esiste | Uso |
|--------|--------|-----|
| **Shared memory** | Sì | Overlay locali (layout AC-compat) |
| **UDP out** | Sì (`UdpTelemetryBridge`, :20777) | Telemetria best-effort LAN |
| **UDP in** | Sì (`UdpTelemetryListener`, :20747) | Stream esterni |
| **yojimbo / netcode** | Parziale (`NetworkManager`) | Multiplayer game state |
| **TCP telemetria** | **No** (fino a questo doc) | — |
| **MQTT** | Stub (`MqttClient`) | Non produzione |

Nel codice **non** c’era ancora un publisher TCP dedicato alla telemetria.
Il multiplayer punta su **UDP affidabile applicativo** (yojimbo), non su TCP grezzo.

---

## Confronto canali (sim racing)

| | Shared memory | UDP | TCP |
|--|---------------|-----|-----|
| **Latenza** | Minima (stesso PC) | Bassa | Più alta (handshake, buffer) |
| **Affidabilità** | N/A locale | No (perdite ok) | Sì (ordine + ritrasmissione) |
| **Remoto** | No | Sì | Sì |
| **Multi-reader** | Eccellente | Broadcast/multicast | 1 connessione = 1 client (o fan-out server) |
| **Rate tipico** | 100–1000 Hz | 20–60 Hz | 10–60 Hz |
| **Casi d’uso** | Dash locale, FFB app | Phone dash, log leggeri | Logger remoto, tool analysis, relay WAN |

Industry practice: **SM** e **UDP** dominano la telemetria live; **TCP** compare per
logging affidabile, remote coaching, o bridge “SM → rete”.

---

## Quando usare TCP in ksim

1. **Logger remoto** su altro PC senza tollerare buchi nei sample
2. **Tool analysis** che fa request/response (es. “dammi ultimo giro”)
3. **Relay** che legge SM e inoltra a cloud / coach
4. **Non** per FFB o HUD a 1 kHz (troppa jitter)

## Quando non usarlo

- FFB / motion (preferire SM o UDP locale)
- Multiplayer gameplay (yojimbo già orientato a lag compensation)
- Broadcast a N app sullo stesso host (SM vince)

---

## Design proposto: `TcpTelemetryBridge`

```
SimulationLoop::tick
  → publishSharedMemory()   // locale
  → publishUdpTelemetry()   // best-effort
  → publishTcpTelemetry()   // solo se client connesso
```

### Protocollo (v1)

Stream **TCP**, little-endian, messaggi con length-prefix:

```
[uint32 le length][payload]

payload = stesso layout di UdpTelemPacket (magic KSIM, version 1)
   oppure
payload = JSON UTF-8 (se jsonMode)
```

- Server: ascolta `0.0.0.0:20778` (default)
- Accetta **un client** (v1); disconnect → torna in listen
- Rate limit: invia al massimo ogni N ms (default 16 ms ≈ 60 Hz) anche se il tick è 1 kHz

### Porte ksim (riepilogo)

| Porta | Protocollo | Direzione |
|-------|------------|-----------|
| 20747 | UDP | In (listener esterno) |
| 20777 | UDP | Out telemetria |
| 20778 | TCP | Out telemetria affidabile |

---

## Multiplayer vs telemetria

| | Game net (yojimbo) | Telemetry TCP |
|--|--------------------|---------------|
| Payload | Input, car state, session | Solo telemetria lettura |
| QoS | Lag compensation, snapshot | Stream monotono sample |
| Sicurezza | Auth netcode | Bind localhost o LAN trusted |

Non mescolare i due: il client di gara non deve dipendere dal canale telemetria.

---

## Roadmap TCP

| Step | Descrizione |
|------|-------------|
| **P0** | Server TCP + length-prefix + `UdpTelemPacket` (questo commit) |
| P1 | Multi-client fan-out |
| P2 | Comandi client (`PING`, `SET_RATE`, `GET_STATIC`) |
| P3 | TLS opzionale / token auth per WAN |
