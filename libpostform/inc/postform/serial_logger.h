
#ifndef POSTFORM_SERIAL_LOGGER_H_
#define POSTFORM_SERIAL_LOGGER_H_

#include <atomic>

#include "ditto/badge.h"
#include "postform/logger.h"

namespace Postform {

template <class T>
class SerialLogger;

template <class T>
class SerialWriter {
 public:
  /**
   * @brief Creates an invalid writer.
   *
   * This writer will not be allowed to write to the transport, even if write
   * and commit are called on it.
   */
  SerialWriter() = default;

  /**
   * @brief Disallow copy, as there should be only a single valid instance of
   * this object.
   */
  SerialWriter(const SerialWriter&) = delete;
  SerialWriter& operator=(const SerialWriter&) = delete;

  /**
   * @brief Creates an object moving from another and invalidating the
   * moved-from object.
   * @param other The object to move from.
   */
  SerialWriter(SerialWriter&& other);

  /**
   * @brief Moves a writer into another, leaving the moved-from writer invalid
   * and releasing the current writer if it was valid. moved-from object.
   * @param other The object to move from.
   * @return a reference to this instance.
   */
  SerialWriter& operator=(SerialWriter&& other);

  /**
   * @brief The destructor will commit changes if the instance is valid,
   *        releasing the writer back to the logger.
   */
  ~SerialWriter();

  /**
   * @brief Writes data to the transport. It frames the data using reverse COBS.
   * @param data Pointer to the data to be written to the transport.
   * @param size size of the data to be written to the transport.
   */
  void write(const uint8_t* data, uint32_t size);

  /**
   * @brief Commits the changes to the transport. This is how we know when the
   * message is done.
   */
  void commit();

  /**
   * @brief Implicit bool conversion
   */
  operator bool() const;

 private:
  /**
   * @brief Possible states of the writer.
   */
  enum class State {
    //! The writer is valid and can write to a transport.
    Writable,
    //! The writer is done with the message and can no longer write to the
    //! transport.
    Finished
  };

  SerialLogger<T>* m_logger = nullptr;
  T* m_transport = nullptr;
  State m_state = State::Finished;
  //! Number of bits since the last 0.
  uint32_t m_marker = 1;

  /**
   * @brief Actual constructor for the SerialWriter. Only created from the
   *        SerialLogger.
   * @param logger Pointer to the logger instance that created this writer.
   * @param transport Pointer to the underlying transport for the writer.
   */
  SerialWriter(SerialLogger<T>* logger, T* transport);

  //! The logger needs to be able to call this private constructor.
  //! It should be the only class able to create a valid instance of this
  //! object.
  friend class SerialLogger<T>;
};

template <class T>
class SerialLogger : public Logger<SerialLogger<T>, SerialWriter<T>> {
 public:
  /**
   * @brief Public constructor for a serial logger. It must be given a transport
   * to write the serialized data.
   * @param transport Pointer to the underlying transport. Must implement the
   * following methods:
   *
   * ```c++
   *   void write(uint8_t value);
   *   void commit();
   * ```
   */
  SerialLogger(T* transport) : m_transport(transport) {}

  /**
   * @brief Gets the writer if it is available. If it is already taken (maybe by
   * some other thread or by our own) we return an invalid instance, since we
   * can't risk writing to the transport or it could scramble the serial data.
   *
   * Can only be called by the Logger as a Badge needs to be provided.
   */
  SerialWriter<T> getWriter(
      Ditto::Badge<Logger<SerialLogger<T>, SerialWriter<T>>>) {
    return getWriter();
  }

  void release(Ditto::Badge<SerialWriter<T>>) { release(); }

 private:
  std::atomic_bool m_taken{false};
  T* m_transport;

  /**
   * @brief Gets the writer if it is available. If it is already taken (maybe by
   * some other thread or by our own) we return an invalid instance, since we
   * can't risk writing to the transport or it could scramble the serial data.
   */
  SerialWriter<T> getWriter();

  /**
   * @brief Releases the writer back to the SerialLogger so that it can be used
   * again later.
   */
  void release();

  friend class SerialLoggerTest;
};

template <class T>
SerialWriter<T>::SerialWriter(SerialWriter<T>&& other)
    : m_logger(other.m_logger),
      m_transport(other.m_transport),
      m_state(other.m_state),
      m_marker(other.m_marker) {
  other.m_logger = nullptr;
  other.m_transport = nullptr;
  other.m_state = State::Finished;
  other.m_marker = 1;
}

template <class T>
SerialWriter<T>& SerialWriter<T>::operator=(SerialWriter<T>&& other) {
  if (&other != this) {
    m_logger = other.m_logger;
    m_transport = other.m_transport;
    m_state = other.m_state;
    m_marker = other.m_marker;
    other.m_logger = nullptr;
    other.m_transport = nullptr;
    other.m_state = State::Finished;
    other.m_marker = -1;
  }
  return *this;
}

template <class T>
SerialWriter<T>::~SerialWriter() {
  commit();
}

template <class T>
SerialWriter<T>::operator bool() const {
  return State::Writable == m_state;
}

template <class T>
void SerialWriter<T>::write(const uint8_t* data, uint32_t size) {
  if (*this) {
    for (uint32_t i = 0; i < size; i++) {
      if (m_marker == 255) {
        // lets insert a virtual zero marker to continue the message
        m_transport->write(m_marker);
        m_marker = 1;
      }

      // Write the current value
      uint8_t value = data[i];
      if (value == 0) {
        // Write the marker instead
        m_transport->write(m_marker);
        m_marker = 1;
      } else {
        m_transport->write(value);
        m_marker++;
      }
    }
  }
}

template <class T>
void SerialWriter<T>::commit() {
  if (*this) {
    m_state = State::Finished;
    m_transport->write(m_marker);
    m_transport->write(0);
    m_transport->commit();
    m_transport = nullptr;
    m_logger->release({});
    m_logger = nullptr;
    m_marker = 0;
  }
}

template <class T>
SerialWriter<T>::SerialWriter(SerialLogger<T>* logger, T* transport)
    : m_logger(logger), m_transport(transport), m_state(State::Writable) {}

template <class T>
SerialWriter<T> SerialLogger<T>::getWriter() {
  if (!m_taken.exchange(true, std::memory_order_relaxed)) {
    return SerialWriter<T>{this, m_transport};
  }
  return SerialWriter<T>{};
}

template <class T>
void SerialLogger<T>::release() {
  m_taken.store(false, std::memory_order_relaxed);
}

}  // namespace Postform

#endif  // POSTFORM_SERIAL_LOGGER_H_
