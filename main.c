#include <ntddk.h>


#define DEVICE_NAME     L"\\Device\\KMDF_Device_Smp"
#define SYMBOLIC_NAME   L"\\??\\KMDF_Device_Smp"

//
// �ں˻ص�����
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
// ����Ӧ�ò��豸�ص����� 
// ��Ӧ�ò�ʹ��CreateFile("\\\\.\\KMDF_Device_Smp",...)��һ���豸ʱ,�������������Ӵ��룬Ȼ��
// ��ȡ����Ӧ���豸����Ȼ��͵õ�������豸����PDEVICE_OBJECT pDevice��һ������MyCreateFunc�Ļ���
// IRP��������Ϣָ��pIrp
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
    // ��ȡIRP����ľ������� 
    // BOOL ReadFile(
    //  HANDLE       hFile,
    //  LPVOID       lpBuffer,
    //  DWORD        nNumberOfBytesToRead,
    //  LPDWORD      lpNumberOfBytesRead,
    //  LPOVERLAPPED lpOverlapped );
    // �����ȡReadFile���ṩ��bufferλ���Լ�buffer��С�����������readBuffer��
    // Ӧ�ò��readFile���ṩ��buffer����ʵ�����ڴ���ͬһ���ַ���������������ں�
    // �ڴ棬Ӧ�ò������������ڴ棬������������ڴ��Ӧ���ں��ڴ���ͬһ�顣
    //

    ULONG readSize = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length; //��ӦnNumberOfBytesToRead

    PCHAR readBuffer = pIrp->AssociatedIrp.SystemBuffer;//��ӦlpBuffer

    
    PCHAR message = "[message from kernel]";

    RtlCopyMemory(readBuffer, message, strlen(message));//������д��������


    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = strlen(message);//��ӦlpNumberOfBytesRead
     
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}


NTSTATUS
MyWriteFunc(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrint("Device get request:write.\n");

    //
    // ��ȡIRP����ľ������� 
    // BOOL WriteFile(
    //  HANDLE       hFile,
    //  LPCVOID      lpBuffer,
    //  DWORD        nNumberOfBytesToWrite,
    //  LPDWORD      lpNumberOfBytesWritten,
    //  LPOVERLAPPED lpOverlapped
    // );
    //

    ULONG writeSize = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length; //��ӦnNumberOfBytesToWrite

    PCHAR writeBuffer = pIrp->AssociatedIrp.SystemBuffer;//��ӦlpBuffer

    RtlZeroMemory(pDevice->DeviceObjectExtension, 200);//���������ڴ����豸ʱ�ṩ��һƬ��ʼ���ڴ�

    RtlCopyMemory(pDevice->DeviceObjectExtension, writeBuffer, writeSize);//�����Ķ����浽��Ƭ�ڴ�

    DbgPrint("%p,%s\n",writeBuffer,(PCHAR)pDevice->DeviceExtension);

    pIrp->IoStatus.Status = status;

    pIrp->IoStatus.Information = writeSize;//��ӦlpNumberOfBytesWrite

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}


//
// ж������ʱ�ĺ���
// �����Ͻ��ذ������õ��Ķ���������������лص� �����豸�������driver�ڴ���device��
// Ҫɾ��device������ɾ��device��symboliclink
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
    // �����豸 
    // ����ͬʱ���й������ڲ�ʹ�õ��豸ָ���Լ����ⲿ���õ��豸��
    //

    status = IoCreateDevice(
                    DriverObject,
                    300,            //����ϵͳΪ����豸��ʼ�����ں��ڴ�Ĵ�С
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
    // Ϊ��д�������û�����
    // ����������Ӧ�ò����Ĳ���ͬһƬ�ڴ�ռ䣬������Ҫע�ⲻҪд����Ч���û��ڴ���
    //

    pDevice->Flags |= DO_BUFFERED_IO;

    //
    // �����豸���ķ�������
    // ���з������Ӻ�����豸���ͷ��Ű󶨣��ɹ�Ӧ�ò�����豸
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
    // �����ں˻ص�����
    // ��ϵͳ���µ��߳�����ʱ���ͻ���������������.����˵����ص������Ǵ��ں˻�
    // ȡ��Ϣ��
    // 

    PsSetCreateProcessNotifyRoutine(ProcessNotifyCallBackFunc, FALSE);


    //
    // ����Ӧ�ò��豸�ص����� 
    // ��Ӧ�ò㽻�� IRP��I/O Request Packet��
    // ��������Ӧ�ò�ʹ��createFile("\\\\.\\KMDF_Device_Smp",...)��һ���豸ʱ��
    // IRP�����������͸��ж���MajorFunction[IRP_MJ_CREATE]������������Ȼ����
    // ��������������豸���ͻ���ö����MajorFunction[IRP_MJ_CREATE]������
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = MyCreateFunc;

    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MyCloseFunc;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MyCleanUpFunc;

    DriverObject->MajorFunction[IRP_MJ_READ] = MyReadFunc;

    DriverObject->MajorFunction[IRP_MJ_WRITE] = MyWriteFunc;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyControlFunc;

    return status;
}
