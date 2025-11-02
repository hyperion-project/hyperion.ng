#include <utils/MemoryTracker.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memory_objects_create, "memory.objects.create");
Q_LOGGING_CATEGORY(memory_objects_track, "memory.objects.track");
Q_LOGGING_CATEGORY(memory_objects_destroy, "memory.objects.destroy");
Q_LOGGING_CATEGORY(memory_non_objects_create, "memory.non_objects.create");
Q_LOGGING_CATEGORY(memory_non_objects_destroy, "memory.non_objects.destroy");