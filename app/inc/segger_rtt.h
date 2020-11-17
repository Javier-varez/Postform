
#ifndef SEGGER_RTT_H_
#define SEGGER_RTT_H_

#include <atomic>
#include <cstdint>
#include <cstring>

class Rtt {
 public:

  enum class Flags: uint32_t {
    BLOCK_IF_FULL = 2,
    NO_BLOCK_TRIM = 1
  };

  struct Channel {
    const char* const name { nullptr };
    std::uint8_t* const buffer = nullptr;
    const std::uint32_t size { 0 };
    std::atomic<std::uint32_t> write { 0 };
    std::atomic<std::uint32_t> read { 0 };
    std::atomic<Flags> flags { Flags::NO_BLOCK_TRIM };

    Channel(const char* name, std::uint8_t* buffer, std::uint32_t size) :
      name(name), buffer(buffer), size(size) { }
  };

  struct Header {
    constexpr static std::uint32_t ID_LENTGH = 16;

    char id[ID_LENTGH];
    std::uint32_t max_up_channels { 1 };
    std::uint32_t max_down_channels { 1 };

    Header() {
      // We need to split this string or otherwise it will be found in memory in .rodata
      // which is not correct
      const char* first_part = "SEGGER";
      const char* second_part = " RTT\0\0\0\0\0";
      constexpr uint32_t FIRST_PART_LENGTH = 6;
      constexpr uint32_t SECOND_PART_LENGTH = 10;
      std::memcpy(id, first_part, FIRST_PART_LENGTH);
      std::memcpy(id + FIRST_PART_LENGTH, second_part, SECOND_PART_LENGTH);
    }
  };

  struct ControlBlock {
    Header header;
    Rtt::Channel up_channel;
    Rtt::Channel down_channel;

    ControlBlock(std::uint8_t* up_buffer, std::uint32_t up_buffer_size,
                  std::uint8_t* down_buffer, std::uint32_t down_buffer_size) :
      header(),
      up_channel("up", up_buffer, up_buffer_size),
      down_channel("down", down_buffer, down_buffer_size) { }
  };

  class Writer {
   public:
    void write(const uint8_t *data, uint32_t size);
    void commit();

    Writer();
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    Writer(Writer&&);
    Writer& operator=(Writer&&);
    ~Writer();

    operator bool() { return State::Writable == m_state; }

   private:
    enum class State {
      Writable,
      Finished
    };

    Rtt* m_rtt = nullptr;
    Channel* m_channel = nullptr;
    uint32_t m_write_ptr = 0;
    State m_state = State::Writable;

    Writer(Rtt* rtt, Channel* channel);
    uint32_t getMaxContiguous() const;

    friend class Rtt;
  };

  static Rtt& getInstance() {
    static Rtt rtt;
    return rtt;
  }

  Writer getWriter();

 private:
  std::atomic<bool> m_taken { false };

  Rtt() = default;
  void releaseWriter() {
    m_taken.store(false);
  }

  friend class Writer;
};

#endif
