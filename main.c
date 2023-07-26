#include <ntddk.h>


#define DEVICE_NAME     L"\\Device\\KMDF_Device_Smp"
#define SYMBOLIC_NAME   L"\\??\\KMDF_Device_Smp"

//
// 内核回调函数
//

VOID
ProcessNotifyCallBackFunc(
    _In_ HANDLE ParentId,
    _In_ HANDLE ProcessId,
    _In_ BOOLEAN Create) 
{
    DbgPrint("ProcessNotifyCallBackFunc.\n");
}

//
// 创建应用层设备回调函数 
// 在应用层使用CreateFile("\\\\.\\KMDF_Device_Smp",...)打开一个设备时,会把这个符号链接传入，然后
// 获取到相应的设备名，然后就得到了这个设备对象PDEVICE_OBJECT pDevice，一起输入MyCreateFunc的还有
// IRP的请求消息指针pIrp
// 

NTSTATUS 
MyCreateFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device has been opened.\n");

    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

 }

NTSTATUS
MyCloseFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device has been closed.\n");

    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

NTSTATUS
MyCleanUpFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device has been clean up.\n");

    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

NTSTATUS
MyReadFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device get request:read.\n");

    //
    // 获取IRP请求的具体内容 
    // BOOL ReadFile(
    //  HANDLE       hFile,
    //  LPVOID       lpBuffer,
    //  DWORD        nNumberOfBytesToRead,
    //  LPDWORD      lpNumberOfBytesRead,
    //  LPOVERLAPPED lpOverlapped );
    // 比如获取ReadFile中提供的buffer位置以及buffer大小。其中这里的readBuffer和
    // 应用层的readFile中提供的buffer的真实物理内存是同一块地址。驱动操作的是内核
    // 内存，应用操作的是虚拟内存，但是这个虚拟内存对应的内核内存是同一块。
    //

    ULONG readSize = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length; //对应nNumberOfBytesToRead

    PCHAR readBuffer = pIrp->AssociatedIrp.SystemBuffer;//对应lpBuffer

    
    PCHAR message = "[message from kernel]";

    RtlCopyMemory(readBuffer, message, strlen(message));//把数据写到缓冲区


    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = strlen(message);//对应lpNumberOfBytesRead
     
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}


NTSTATUS
MyWriteFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device get request:write.\n");

    //
    // 获取IRP请求的具体内容 
    // BOOL WriteFile(
    //  HANDLE       hFile,
    //  LPCVOID      lpBuffer,
    //  DWORD        nNumberOfBytesToWrite,
    //  LPDWORD      lpNumberOfBytesWritten,
    //  LPOVERLAPPED lpOverlapped
    // );
    //

    ULONG writeSize = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length; //对应nNumberOfBytesToWrite

    PCHAR writeBuffer = pIrp->AssociatedIrp.SystemBuffer;//对应lpBuffer

    RtlZeroMemory(pDevice->DeviceObjectExtension, 200);//利用我们在创建设备时提供的一片初始化内存

    RtlCopyMemory(pDevice->DeviceObjectExtension, writeBuffer, writeSize);//读到的东西存到这片内存

    DbgPrint("%p,%s\n",writeBuffer,(PCHAR)pDevice->DeviceExtension);

    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = writeSize;//对应lpNumberOfBytesWrite

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}


//
// 卸载驱动时的函数
// 必须严谨地把所有用到的东西都清除掉，所有回调 所有设备如果有在driver内创建device，
// 要删除device，并且删除device的symboliclink
//

VOID
DriverUnloadFunc(_Inout_ PDRIVER_OBJECT DriverObject)
{
    DbgPrint("Driver Unloaded. \n");

    if (DriverObject->DeviceObject) 
    {
        IoDeleteDevice(DriverObject->DeviceObject);
        
        UNICODE_STRING symbolicName = { 0 };

        RtlInitUnicodeString(&symbolicName, SYMBOLIC_NAME);

        IoDeleteSymbolicLink(&symbolicName);
    }

    PsSetCreateProcessNotifyRoutine(ProcessNotifyCallBackFunc, TRUE);
}


NTSTATUS 
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING deviceName = { 0 };
    PDEVICE_OBJECT pDevice = NULL;

    DriverObject->DriverUnload = DriverUnloadFunc;

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);

    //
    // 创建设备 
    // 这里同时会有供驱动内部使用的设备指针以及供外部调用的设备名
    //

    status = IoCreateDevice(
                    DriverObject,
                    300,            //设置系统为这个设备初始化的内核内存的大小
                    &deviceName,
                    FILE_DEVICE_UNKNOWN,
                    0,
                    TRUE,
                    &pDevice);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("Error:IoCreateDevice.%x\n", status);

        return status;
    }

    //
    // 为读写操作设置缓冲区
    // 由于驱动和应用操作的不是同一片内存空间，所以需要注意不要写到无效的用户内存区
    //

    pDevice->Flags |= DO_BUFFERED_IO;

    //
    // 创建设备名的符号连接
    // 进行符号链接后，这个设备名和符号绑定，可供应用层调用设备
    //

    UNICODE_STRING symbolicName = { 0 };

    RtlInitUnicodeString(&symbolicName, SYMBOLIC_NAME);

    status = IoCreateSymbolicLink(&symbolicName,&deviceName);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("Error: IoCreateSymbolicLink.%x\n", status);

        IoDeleteDevice(pDevice);

        return status;
    }

    //
    // 设置内核回调函数
    // 当系统有新的线程启动时，就会调用这个驱动函数.等于说这个回调函数是从内核获
    // 取信息的
    // 

    PsSetCreateProcessNotifyRoutine(ProcessNotifyCallBackFunc, FALSE);


    //
    // 设置应用层设备回调函数 
    // 与应用层交互 IRP（I/O Request Packet）
    // 当我们在应用层使用createFile("\\\\.\\KMDF_Device_Smp",...)打开一个设备时，
    // IRP会把这个请求发送给有定义MajorFunction[IRP_MJ_CREATE]函数的驱动，然后如
    // 果该驱动有这个设备，就会调用定义的MajorFunction[IRP_MJ_CREATE]函数。
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreateFunc;

    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyCloseFunc;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MyCleanUpFunc;

    DriverObject->MajorFunction[IRP_MJ_READ] = MyReadFunc;

    DriverObject->MajorFunction[IRP_MJ_WRITE] = MyWriteFunc;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyControlFunc;

    return status;
}
