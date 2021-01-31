
#ifndef POSTFORM_TYPES_H_
#define POSTFORM_TYPES_H_

namespace Postform {

/**
 * @brief Internal representation of an interned string.
 *
 * This is serialized as a pointer, instead of copying
 * the whole string through the transport.
 */
struct InternedString {
  const char* str;
};

}  // namespace Postform

#endif  // POSTFORM_TYPES_H_
