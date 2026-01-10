# Shared-Memory-Driver

Shared Memory Driver exploiting a Registery Callback ( `CmRegisterCallbackEx` ) for communication using a MDL. <br />
It creates a Registry Key ( `SOFTWARE\\SharedMemory` ) and writes two different Values to the Registry Key for communication.  <br />
- **InitializeSharedMemory**: Creates a MDL ( Passes `data_initialize_t` containg `data_request_t` pointer and Process ID )
- **RequestSharedMemory**: Handles requests ( Accesses the MDL of the pointer to `data_request_t` and handles requests )
