#include "register_types.h"

#include "gdextension/core/execution/gd_dispatcher.h"
#include "system_node.h"

#include "core/execution/scheduler.hpp"

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <memory>


using namespace godot;

void initialize_helium_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_CLASS(he::system_node);

    he::exec::scheduler::instance().set_dispatcher(std::make_unique<he::gd_dispatcher>());
}

void uninitialize_helium_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
}

extern "C"
{
GDExtensionBool GDE_EXPORT helium_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                   GDExtensionClassLibraryPtr p_library,
                                                   GDExtensionInitialization* r_initialization)
{
    ::godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_helium_module);
    init_obj.register_terminator(uninitialize_helium_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
