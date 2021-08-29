#include <KLVStorage.h>

using namespace KLV;

KLVStorage::KLVStorage() {
}

KLVStorage::~KLVStorage() {
    for (auto &pair : this->data) {
        g_free(pair.second);
    }
}

bool KLVStorage::get(guint64 key, GValue *res) {
    auto pair = this->data.find(key);
    if (pair == this->data.end()) {
        return false;
    }
    g_value_copy(pair->second, res);
    return true;
}

bool KLVStorage::take(guint64 key, GValue *res) {
    auto pair = this->data.find(key);
    if (pair == this->data.end()) {
        return false;
    }
    g_value_copy(pair->second, res);
    g_free(pair->second);
    this->data.erase(pair);
    return true;
}

bool KLVStorage::remove(guint64 key) {
    auto pair = this->data.find(key);
    if (pair == this->data.end()) {
        return false;
    }
    g_free(pair->second);
    this->data.erase(pair);
    return true;
}

bool KLVStorage::set(guint64 key, GValue *res) {
    GValue *holder = g_new(GValue, 1);
    *holder = G_VALUE_INIT;
    g_value_unset(holder);
    g_value_init(holder, G_VALUE_TYPE(res));
    g_value_copy(res, holder);
    this->data.emplace(key, holder);
    return true;
}