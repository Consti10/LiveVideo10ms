#ifndef SHARED_QUEUE_HH
#define SHARED_QUEUE_HH

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename tmpl__T>
class SharedQueue {
public:
  SharedQueue(size_t max_size, bool clear_on_full = false)
  : m_max_size(max_size), m_clear_on_full(clear_on_full) {}

  tmpl__T pop() {
    std::unique_lock<std::mutex> lock_guard(m_mutex);

    while (m_queue.empty()) {
      m_cond.wait(lock_guard);
    }

    auto item = m_queue.front();
    m_queue.pop_front();
    return item;
  }

  void push(tmpl__T item) {
    std::unique_lock<std::mutex> lock_guard(m_mutex);
    if ((m_queue.size() >= m_max_size) && m_clear_on_full) {
      m_queue.clear();
    }
    if (m_queue.size() < m_max_size) {
      m_queue.push_back(item);
    }
    lock_guard.unlock();
    m_cond.notify_one();
  }

  size_t size() const {
    return m_queue.size();
  }

private:
  size_t m_max_size;
  bool m_clear_on_full;
  std::deque<tmpl__T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};

#endif // SHARED_QUEUE_HH
