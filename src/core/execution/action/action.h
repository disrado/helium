#pragma once

#include "action_base.hpp"


namespace he
{

class action: public action_base
{
public:
    struct exec_token final
    {
    public:
        auto succeed() -> void;
        auto fail() -> void;

        auto is_succeeded() const -> bool;

    private:
        bool _succeeded{ false };
    };

public:
    action() = default;
    explicit action(delegate<exec_token&, const context&> definition, std::optional<context> initial_context = std::nullopt);

    auto execute() -> void override;
    auto abort() -> void override;

private:
    delegate<exec_token&, const context&> _definition;

    exec_token _exec_token;
};

}
