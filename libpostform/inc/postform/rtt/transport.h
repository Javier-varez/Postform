
#ifndef POSTFORM_RTT_TRANSPORT_H_
#define POSTFORM_RTT_TRANSPORT_H_

#include "postform/rtt/rtt.h"

namespace Postform::Rtt {

class Transport {
 public:
  /**
   * @brief Constructs an Rtt transport to transmit information over the RTT up
   * channel
   *
   * @param channel The RTT UP channel over which this transport will send
   * information back to the host.
   */
  Transport(Channel* channel);

  /**
   * @brief writes data to the RTT up channel.
   */
  void write(uint8_t value);

  /**
   * @brief Commits the data to the RTT up channel once a message is complete.
   */
  void commit();

 private:
  Channel* m_channel = nullptr;
  uint32_t m_write_ptr = 0;

  uint32_t getNextWritePtr() const;
};

}  // namespace Postform::Rtt

#endif  // POSTFORM_RTT_TRANSPORT_H_
