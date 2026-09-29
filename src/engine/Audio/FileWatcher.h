#pragma once

// QFileSystemWatcher replacement: polls the watched file's mtime on a
// background thread and fires the callback once per change (Qt fired
// fileChanged(path); the directory-level directoryChanged() is not needed by
// the audio module). Callbacks run on the watcher thread, exactly like the Qt
// signal fired on the event-loop thread.

#include "AudioUtil.h"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace ks::audio {

class FileWatcher {
public:
    using Callback = std::function<void(const std::string& path)>;

    FileWatcher() = default;
    ~FileWatcher() { unwatch(); }

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    // QFileSystemWatcher::addPath(): begins polling; fires callback whenever
    // the file's mtime or existence changes.
    void watch(const std::string& path, Callback callback, int intervalMs = 500)
    {
        unwatch();
        if (path.empty()) return;
        m_path = path;
        m_callback = std::move(callback);
        m_stop = false;
        m_thread = std::thread([this, intervalMs]() {
            namespace fs = std::filesystem;
            std::error_code ec;
            auto lastWrite = fs::last_write_time(fs::path(m_path), ec);
            bool lastExists = !ec;
            const bool hadError = ec != std::error_code();
            (void)hadError;
            while (!m_stop.load()) {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait_for(lock, std::chrono::milliseconds(intervalMs),
                              [this]() { return m_stop.load(); });
                if (m_stop.load()) break;
                lock.unlock();

                std::error_code ec2;
                const auto writeTime = fs::last_write_time(fs::path(m_path), ec2);
                const bool exists = !ec2;
                const bool changed = (exists != lastExists) || (exists && writeTime != lastWrite);
                lastExists = exists;
                lastWrite = writeTime;
                if (changed && m_callback) m_callback(m_path);
            }
        });
    }

    // QFileSystemWatcher::removePath() / clear()
    void unwatch()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        if (m_thread.joinable()) m_thread.join();
        m_path.clear();
        m_callback = nullptr;
        m_stop = false;
    }

    bool isWatching() const { return !m_path.empty(); }

private:
    std::string m_path;
    Callback m_callback;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stop{false};
};

} // namespace ks::audio
