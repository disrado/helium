#include "run.hpp"


namespace he
{

auto run::cancel() const -> void
{
    _graph->cancel();
}

}
