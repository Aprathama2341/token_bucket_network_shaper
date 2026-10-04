# Testing and Demonstration Procedure

## 1. Build

```bash
cd driver
make
cd ../daemon
make
cd ..
```

## 2. Load Driver

```bash
sudo insmod driver/tbq_driver.ko
ls -l /dev/tbqueue
```

Expected: a device named `/dev/tbqueue` exists.

## 3. Start Daemon

```bash
sudo chmod 666 /dev/tbqueue
./daemon/token_bucket_daemon
```

## 4. Driver Queue Test

```text
reset
shape 5000
shape 7000
stats
```

The kernel statistics should show increased queued/enqueued byte counts.

## 5. Shaping Test

```text
rate 0
shape 8000
stats
```

The packet should remain queued because no new tokens are generated.

Then:

```text
rate 100000
tick
stats
```

The queued packet should become eligible for transmission.

## 6. Policing Test

```text
reset
burst 10000
police 7000
police 7000
stats
```

At least one request should be rejected when the available token balance is insufficient.

## 7. Dynamic Configuration Test

```text
rate 50000
burst 5000
stats
rate 200000
burst 20000
stats
```

The daemon should update the token pool parameters without restarting the program.

## 8. Cleanup

Exit the daemon and unload the module:

```bash
sudo rmmod tbq_driver
```

## 9. Evidence for Evaluation

Capture terminal screenshots showing:

- successful kernel-module compilation
- `insmod` and `/dev/tbqueue`
- daemon startup
- shaping with rate 0
- queue statistics
- dynamic rate change
- policer pass/drop behavior
- final GitHub repository
