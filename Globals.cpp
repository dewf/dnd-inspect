#include "Globals.h"

#include <InterfaceKit.h>

const char *K_FIELD_CLIP_NAME = "be:clip_name";
const char *K_FIELD_ACTIONS = "be:actions";
const char *K_FIELD_ORIGINATOR = "be:originator";
const char *K_FIELD_TYPES = "be:types";
const char *K_FIELD_FILETYPES = "be:filetypes";
const char *K_FIELD_TYPE_DESCS = "be:type_descriptions";

const char *K_FIELD_DEFAULT = "xx:KFD";
const char *K_FIELD_DROPWINDOW = "xx:KFDWin";

const char *getActionString(int32 action)
{
    switch(action) {
    case B_COPY_TARGET:
        return "B_COPY_TARGET";
    case B_MOVE_TARGET:
        return "B_MOVE_TARGET";
    case B_LINK_TARGET:
        return "B_LINK_TARGET";
    case B_TRASH_TARGET:
        return "B_TRASH_TARGET";
    }
    return "(unknown)";
}
