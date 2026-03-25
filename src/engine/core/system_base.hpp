#pragma once


namespace he
{


class system_base
{
public:
    virtual ~system_base() = default;

public:
    virtual auto tick(double dt) -> void {
    }
};

}
