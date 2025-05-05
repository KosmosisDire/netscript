#pragma once

namespace ScriptAPI
{
    public ref class Script abstract
    {
    public:
        virtual void Update() {};
        virtual void Start() {};

        // Changed to protected: Accessible by derived classes (like MyFirstScript)
    protected:
        // Returns the entity ID associated with this script instance
        int GetEntityId();

        // Changed back to internal: Only accessible within the ScriptAPI assembly
        // (EngineInterface will call this)
    internal:
        void SetEntityId(int id);

    private:
        int entityId = -1;
    };
} // namespace ScriptAPI