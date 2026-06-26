#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace teamforge {

template <typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value)); }
    static Result err(std::string message) { return Result(std::move(message)); }

    [[nodiscard]] bool is_ok() const { return std::holds_alternative<T>(data_); }
    [[nodiscard]] bool is_err() const { return std::holds_alternative<std::string>(data_); }

    T& value() { return std::get<T>(data_); }
    const T& value() const { return std::get<T>(data_); }

    const std::string& error() const { return std::get<std::string>(data_); }

    T value_or(T fallback) const {
        if (is_ok()) return std::get<T>(data_);
        return fallback;
    }

private:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(std::string error) : data_(std::move(error)) {}

    std::variant<T, std::string> data_;
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
