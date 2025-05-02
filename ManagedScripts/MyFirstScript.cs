using System; // Required for Console

// Optional namespace for organization
namespace ManagedScripts
{
    public class MyFirstScript
    {
        // Static method to be called from C++/CLI via Reflection
        public static void SayHello(string name)
        {
            Console.WriteLine($"---> Hello {name} from MyFirstScript.SayHello (C#)!");
        }

        // Instance method placeholder for later script execution
        public void Update()
        {
            Console.WriteLine($"---> MyFirstScript Update! (Instance Method)");
            // In later phases, this would access engine components
        }

        // Constructor (optional)
        public MyFirstScript()
        {
             Console.WriteLine($"---> MyFirstScript instance created.");
        }
    }
}