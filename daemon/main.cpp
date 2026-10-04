#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../driver/tbq_ioctl.h"

class TokenBucket {
public:
    using Clock = std::chrono::steady_clock;
    TokenBucket(double rate, double burst)
        : rate_(std::max(0.0, rate)), burst_(std::max(0.0, burst)),
          tokens_(burst_), last_(Clock::now()) {}

    void refill() {
        auto now = Clock::now();
        std::chrono::duration<double> elapsed = now - last_;
        tokens_ = std::min(burst_, tokens_ + elapsed.count() * rate_);
        last_ = now;
    }

    bool consume(std::uint64_t bytes) {
        refill();
        if (static_cast<double>(bytes) > tokens_) return false;
        tokens_ -= static_cast<double>(bytes);
        return true;
    }

    void setRate(double rate) { refill(); rate_ = std::max(0.0, rate); }
    void setBurst(double burst) {
        refill(); burst_ = std::max(0.0, burst); tokens_ = std::min(tokens_, burst_);
    }
    double tokens() { refill(); return tokens_; }
    double rate() const { return rate_; }
    double burst() const { return burst_; }

private:
    double rate_, burst_, tokens_;
    Clock::time_point last_;
};

class ShaperDaemon {
public:
    explicit ShaperDaemon(int fd) : fd_(fd), bucket_(100000.0, 20000.0) {}

    bool submit(std::uint64_t bytes) {
        if (bytes == 0) return false;
        // Driver accounts for bytes in the virtual kernel queue.
        if (::write(fd_, &bytes, sizeof(bytes)) != static_cast<ssize_t>(sizeof(bytes))) {
            ++queueSubmitDrops_;
            return false;
        }
        queue_.push_back(bytes);
        drain();
        return true;
    }

    bool police(std::uint64_t bytes) {
        if (bytes == 0 || !bucket_.consume(bytes)) {
            ++policerDrops_;
            return false;
        }
        ++policerPasses_;
        return true;
    }

    void drain() {
        bucket_.refill();
        while (!queue_.empty()) {
            auto bytes = queue_.front();
            if (!bucket_.consume(bytes)) break;
            if (::ioctl(fd_, TBQ_IOC_DEQUEUE, &bytes) < 0) {
                std::cerr << "Driver dequeue failed.\n";
                break;
            }
            queue_.pop_front();
        }
    }

    void stats() {
        drain();
        tbq_stats s{};
        if (::ioctl(fd_, TBQ_IOC_GET_STATS, &s) < 0) {
            perror("TBQ_IOC_GET_STATS"); return;
        }
        std::cout << std::fixed << std::setprecision(1)
                  << "\n=== Token Bucket ===\n"
                  << "Rate: " << bucket_.rate() << " bytes/sec\n"
                  << "Burst: " << bucket_.burst() << " bytes\n"
                  << "Tokens: " << bucket_.tokens() << " bytes\n"
                  << "\n=== Kernel Virtual Queue ===\n"
                  << "Queued bytes: " << s.queued_bytes << "\n"
                  << "Enqueued packets: " << s.enqueued_packets << "\n"
                  << "Transmitted packets: " << s.transmitted_packets << "\n"
                  << "Queue drops: " << s.queue_drops << "\n"
                  << "Max queue bytes: " << s.max_queue_bytes << "\n"
                  << "User queue packets: " << queue_.size() << "\n"
                  << "\n=== Policer ===\n"
                  << "Passes: " << policerPasses_ << "\n"
                  << "Drops: " << policerDrops_ << "\n"
                  << "Driver submission failures: " << queueSubmitDrops_ << "\n";
    }

    void rate(double v) { bucket_.setRate(v); drain(); }
    void burst(double v) { bucket_.setBurst(v); drain(); }
    void tick() { drain(); }
    void reset() {
        queue_.clear();
        if (::ioctl(fd_, TBQ_IOC_RESET) < 0) perror("TBQ_IOC_RESET");
        bucket_ = TokenBucket(100000.0, 20000.0);
        policerPasses_ = policerDrops_ = queueSubmitDrops_ = 0;
    }

private:
    int fd_;
    TokenBucket bucket_;
    std::deque<std::uint64_t> queue_;
    std::uint64_t policerPasses_ = 0, policerDrops_ = 0, queueSubmitDrops_ = 0;
};

static void help() {
    std::cout << "Commands:\n"
              << "  shape <bytes>       Submit packet to virtual kernel queue\n"
              << "  police <bytes>      Police packet immediately\n"
              << "  rate <bytes/sec>    Change token refill rate\n"
              << "  burst <bytes>       Change bucket capacity\n"
              << "  tick                Refill and transmit eligible queued packets\n"
              << "  stats               Show daemon + driver statistics\n"
              << "  reset               Reset queue and counters\n"
              << "  help                Show commands\n"
              << "  quit                Exit\n";
}

int main() {
    int fd = ::open("/dev/tbqueue", O_RDWR);
    if (fd < 0) { perror("/dev/tbqueue"); return 1; }

    ShaperDaemon daemon(fd);
    std::cout << "Token-Bucket Network Bandwidth Shaper & Traffic Policer\n"
              << "Linux kernel virtual queue + C++17 token daemon\n";
    help();

    std::string line;
    while (std::cout << "tbq> " && std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd.empty()) continue;
        if (cmd == "quit" || cmd == "exit") break;
        if (cmd == "help") { help(); continue; }
        if (cmd == "stats") { daemon.stats(); continue; }
        if (cmd == "tick") { daemon.tick(); continue; }
        if (cmd == "reset") { daemon.reset(); continue; }

        double value;
        if (cmd == "rate" || cmd == "burst") {
            if (!(in >> value) || value < 0) { std::cout << "Enter a non-negative number.\n"; continue; }
            if (cmd == "rate") daemon.rate(value); else daemon.burst(value);
            continue;
        }

        if (cmd == "shape" || cmd == "police") {
            std::uint64_t bytes;
            if (!(in >> bytes) || bytes == 0) { std::cout << "Enter bytes > 0.\n"; continue; }
            bool ok = (cmd == "shape") ? daemon.submit(bytes) : daemon.police(bytes);
            std::cout << (ok ? "ACCEPTED\n" : "DROPPED/BLOCKED\n");
            continue;
        }
        std::cout << "Unknown command. Type help.\n";
    }
    ::close(fd);
    return 0;
}
