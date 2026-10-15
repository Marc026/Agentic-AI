#pragma once

#include <string>
#include <utility>

namespace teamforge {

template <typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.ok_ = true;
        r.value_ = std::move(value);
        return r;
    }

    static Result err(std::string message) {
        Result r;
        r.error_ = std::move(message);
        return r;
    }

    [[nodiscard]] bool is_ok() const { return ok_; }
    [[nodiscard]] bool is_err() const { return !ok_; }

    T& value() { return value_; }
    const T& value() const { return value_; }

    const std::string& error() const { return error_; }

    T value_or(T fallback) const {
        if (ok_) return value_;
        return fallback;
    }

private:
    bool ok_{false};
    T value_{};
    std::string error_;
};

template <>
class Result<void> {
public:
    static Result ok() { return Result(true); }
    static Result err(std::string message) { return Result(std::move(message)); }

    [[nodiscard]] bool is_ok() const { return ok_; }
    [[nodiscard]] bool is_err() const { return !ok_; }
    const std::string& error() const { return error_; }

private:
    explicit Result(bool) : ok_(true) {}
    explicit Result(std::string error) : ok_(false), error_(std::move(error)) {}

    bool ok_{false};
    std::string error_;
};

}  // namespace teamforge
