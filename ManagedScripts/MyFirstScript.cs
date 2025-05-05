using System;
// Reference the ScriptAPI namespace to access the base class
using ScriptAPI;

namespace ManagedScripts
{
    // Inherit from the abstract Script class defined in ScriptAPI
    public class MyFirstScript : Script
    {
        private int updateCount = 0;

        // Optional: Override the Start method
        public override void Start()
        {
            Console.WriteLine($"---> MyFirstScript Start() called for Entity ID: {GetEntityId()}");
        }

        // Override the Update method for per-frame logic
        public override void Update()
        {
            updateCount++;
            // Print less frequently to avoid flooding console
            if (updateCount % 60 == 0) // Print roughly once per second if running at 60fps
            {
                //  Console.WriteLine($"---> MyFirstScript Update()! Entity ID: {GetEntityId()}, Count: {updateCount}");
                Console.WriteLine("HELLOOOOO");
            }
        } 
 
        // Constructor (optional)
        public MyFirstScript()
        {
             Console.WriteLine($"---> MyFirstScript instance created (Constructor). Entity ID not set yet.");
        }

        // Remove the old static method
        // public static void SayHello(string name) { ... }
    }
}