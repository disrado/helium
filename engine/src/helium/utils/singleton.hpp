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

public:
    static auto instance() -> t&
    {
        static auto object_instance{ t{} };
        return object_instance;
    }
};

}
