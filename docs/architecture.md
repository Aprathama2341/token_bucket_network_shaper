# Architecture and Design

## 1. Objective

Build a Linux software architecture that controls packet traffic using a Token-Bucket algorithm. The kernel component provides a virtual network queue and byte accounting; the C++ daemon performs rate limiting and dynamic token-pool management.

## 2. Data Flow

1. A packet is represented by its byte count.
2. The C++ daemon submits that byte count to `/dev/tbqueue`.
3. The kernel driver checks virtual queue capacity and records the bytes.
4. The daemon's token bucket determines whether the packet can be transmitted.
5. If enough tokens exist, the daemon issues `TBQ_IOC_DEQUEUE`.
6. If not, the packet remains in the daemon's FIFO queue until tokens refill.
7. The policer bypasses queue waiting and immediately accepts or drops based on available tokens.

## 3. Token-Bucket Model

Let:

- `R` = token refill rate in bytes/second
- `B` = bucket capacity in bytes
- `T` = current tokens
- `Δt` = elapsed time
- `P` = packet size in bytes

Refill:

`T = min(B, T + R × Δt)`

Transmit/accept:

`if P <= T: T = T - P`

Otherwise the shaper waits and the policer drops.

## 4. Driver/User Boundary

The driver is kernel-space C. The daemon is user-space C++. They communicate through a character-like misc device:

- `write()` → enqueue/account packet bytes
- `TBQ_IOC_DEQUEUE` → remove transmitted bytes
- `TBQ_IOC_GET_STATS` → read queue statistics
- `TBQ_IOC_SET_MAX_QUEUE` → configure queue capacity
- `TBQ_IOC_RESET` → clear state

This demonstrates Linux device-driver concepts while keeping the virtual queue deterministic for capstone evaluation.
