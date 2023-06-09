#ifndef _CMD_SHOWFOUND_COMMON_H_
#define _CMD_SHOWFOUND_COMMON_H_ 1

enum ImageMode {
  IMAGE_ALL_ON,
  IMAGE_ALL_OFF,
  IMAGE_FOCUS_ON,
  IMAGE_LAST,
};

enum SkipMode {
  EVERY_PIXEL,
  ONLY_THIS,
  ONLY_OTHER,
  ONLY_UNKNOWN,
  SKIP_LAST,
};

#endif  // _CMD_SHOWFOUND_COMMON_H_
