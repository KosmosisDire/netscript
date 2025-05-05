#include "pch.h" // Include precompiled header first
#include "script.hxx"

namespace ScriptAPI
{
    void Script::SetEntityId(int id)
    {
        this->entityId = id;
    }

    int Script::GetEntityId()
    {
        return this->entityId;
    }

} // namespace ScriptAPI