#include "run.hpp"


namespace he
{

auto run::execute() const -> void
{
    _target->execute();
}

auto run::abort() const -> void
{
    _target->abort();
}

}
