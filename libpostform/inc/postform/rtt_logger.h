
#ifndef POSTFORM_RTT_LOGGER_H_
#define POSTFORM_RTT_LOGGER_H_

#include "postform/logger.h"
#include "postform/rtt/cobs_writer.h"
#include "postform/rtt/rtt_manager.h"

namespace Postform {

class RttLogger: public Logger<RttLogger, Rtt::CobsWriter> {
 public:
  RttLogger() = default;

 private:
  Rtt::CobsWriter getWriter() {
    auto& manager = Rtt::Manager::getInstance();
    return manager.getCobsWriter();
  }

  friend Logger<RttLogger, Rtt::CobsWriter>;
};

}  // namespace Postform

#endif  // POSTFORM_RTT_LOGGER_H_
