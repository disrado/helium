#pragma once


namespace he
{

template <typename t>
class singleton
{
public:
    virtual ~singleton() = default;

protected:
    singleton() = default;

    singleton(const singleton&) = delete;
    auto operator =(const singleton&) -> singleton& = delete;

    singleton(singleton&&) = delete;
    auto operator =(singleton&&) -> singleton& = delete;

public:
    static auto instance() -> t&
    {
        static auto object_instance{ t{} };
        return object_instance;
    }
};

}
