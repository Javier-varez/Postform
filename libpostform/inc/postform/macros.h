
#ifndef POSTFORM_MACROS_H_
#define POSTFORM_MACROS_H_

#define __POSTFORM_STRINGIFY(X) #X
#define __POSTFORM_EXPAND_AND_STRINGIFY(X) __POSTFORM_STRINGIFY(X)

//! Gets the 16th element passed to it
#define POSTFORM_INTERNAL_INTERNAL_16TH(_1, _2, _3, _4, _5, _6, _7, _8, _9, \
                                        _10, _11, _12, _13, _14, _15, _16,  \
                                        ...)                                \
  _16

#define POSTFORM_CAT_I(a, b) a##b

//! Concatenates two elements
#define POSTFORM_CAT(a, b) POSTFORM_CAT_I(a, b)

//! Gets the number of arguments passed into it
#define POSTFORM_NARG(...)                                                     \
  POSTFORM_INTERNAL_INTERNAL_16TH(dummy, ##__VA_ARGS__, 14, 13, 12, 11, 10, 9, \
                                  8, 7, 6, 5, 4, 3, 2, 1, 0)

//! defines a comma, used for other macros for later expansion
#define POSTFORM_COMMA ,

//! Expands to a comma if __VA_ARGS__ is not empty. Otherwise it expands to
//! nothing
#define POSTFORM_EXPAND_COMMA(...)                                          \
  POSTFORM_INTERNAL_INTERNAL_16TH(                                          \
      dummy, ##__VA_ARGS__, POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA, \
      POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA,       \
      POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA,       \
      POSTFORM_COMMA, POSTFORM_COMMA, POSTFORM_COMMA, )

#endif  // POSTFOR_MACROS_H_
