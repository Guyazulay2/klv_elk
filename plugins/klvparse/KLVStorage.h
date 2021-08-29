#pragma once
#include <KLVItem.h>

#include <unordered_map>

namespace KLV {

class KLVStorage {
   public:
    std::unordered_map<guint64, GValue *> data;

    bool get(guint64 key, GValue *res);
    bool take(guint64 key, GValue *res);
    bool remove(guint64 key);
    bool set(guint64 key, GValue *res);
    KLVStorage();
    virtual ~KLVStorage();
};

}  // namespace KLV