# Token-Bucket Network Bandwidth Shaper & Traffic Policer

## Overview

A Linux-based bandwidth-control system that combines a virtual network queue driver with a C++17 token-bucket controller.

The project demonstrates byte-level traffic accounting, dynamic rate limiting, traffic shaping, and traffic policing.

## System Architecture

```text
Packet Byte-Count Event
          |
          v
+-------------------------+
| Linux Virtual Queue     |
| Driver                  |
|                         |
| - Queue accounting      |
| - Byte counters         |
| - Drop statistics       |
+------------+------------+
             |
             | write() / ioctl()
             v
+-------------------------+
| C++17 Token-Bucket      |
| Controller              |
|                         |
| - Token refill          |
| - Token consumption     |
| - Rate / burst control  |
| - SHAPER / POLICER      |
+------------+------------+
             |
             v
       ALLOW / QUEUE / DROP
```

## Token-Bucket Algorithm

Tokens are refilled according to:

```text
T = min(C, T + R × Δt)
```

Where:

- `T` = current number of tokens
- `C` = bucket capacity
- `R` = token rate
- `Δt` = elapsed time
- `B` = packet byte count

If `T >= B`, the packet is allowed and `B` tokens are consumed.

If `T < B`:

- **SHAPER** retains/queues the traffic for controlled release.
- **POLICER** drops the traffic and records the event.

## Project Structure

```text
token-bucket-network-shaper/
├── .gitignore
├── README.md
├── daemon/
│   ├── main.cpp
│   └── Makefile
├── driver/
│   ├── tbq_driver.c
│   ├── tbq_ioctl.h
│   └── Makefile
├── docs/
│   ├── architecture.md
│   └── testing.md
└── scripts/
    ├── setup.sh
    └── cleanup.sh
```

## Technologies

- Linux
- C
- C++17
- Linux Kernel Module
- Token-Bucket Algorithm
- Make
- Device file / ioctl interface

## Key Features

- Virtual network queue driver
- Packet byte-count tracking
- Token-bucket rate limiting
- Configurable rate and burst capacity
- Traffic shaping
- Traffic policing
- Queue and drop statistics
- Runtime control through a C++17 controller

## Build

Build the driver:

```bash
cd driver
make
```

Build the C++ controller:

```bash
cd daemon
make
```

Refer to `docs/architecture.md` and `docs/testing.md` for the project design and validation flow.

## Scope

This capstone uses a **virtual network queue driver** for demonstration and evaluation. It does not claim direct interception or control of physical Wi-Fi/Ethernet traffic.

## Documentation

- `docs/architecture.md` — system architecture
- `docs/testing.md` — testing and validation
- `scripts/setup.sh` — setup helper
- `scripts/cleanup.sh` — cleanup helper

## Capstone

**Token-Bucket Network Bandwidth Shaper & Traffic Policer**  
Linux kernel module + C++17 token-bucket controller.
