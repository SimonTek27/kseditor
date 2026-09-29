#pragma once

// QTimer replacement. The engine has no Qt event loop, so timers run on a
// dedicated background thread (like QBasicTimer did inside a living event
// loop): start(intervalMs, fn) repeats until stop(), startSingle(delayMs, fn)
// fires once (QTimer::singleShot / setSingleShot(true)).

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace ks::audio {

class Timer {
public:
    Timer() = default;
    ~Timer() { stop(); }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    // Repeating timer (QTimer::start(intervalMs)).
    void start(int intervalMs, std::function<void()> callback)
    {
        restart(intervalMs, std::move(callback), false);
    }

    // One-shot timer (QTimer::singleShot(delayMs, lambda)).
    void startSingle(int delayMs, std::function<void()> callback)
    {
        restart(delayMs, std::move(callback), true);
    }

    // QTimer::stop()
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopRequested = true;
            m_active = false;
        }
        m_cv.notify_all();
        if (m_thread.joinable()) m_thread.join();
    }

    // QTimer::isActive()
    bool isActive() const { return m_active.load(); }

private:
    void restart(int intervalMs, std::function<void()> callback, bool singleShot)
    {
        stop();
        if (intervalMs < 0) intervalMs = 0;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = false;
        m_active = true;
        m_thread = std::thread([this, intervalMs, callback, singleShot]() {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_stopRequested) {
                if (m_cv.wait_for(lock, std::chrono::milliseconds(intervalMs),
                                  [this]() { return m_stopRequested; })) {
                    break;
                }
                if (m_stopRequested) break;
                if (singleShot) m_active = false;
                // Run the callback without holding the lock so stop() from
                // inside the callback cannot deadlock.
                auto cb = callback;
                lock.unlock();
                if (cb) cb();
                lock.lock();
                if (singleShot) break;
            }
            m_active = false;
        });
    }

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopRequested = false;
    std::atomic<bool> m_active{false};
};

} // namespace ks::audio
