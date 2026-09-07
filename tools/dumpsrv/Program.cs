using DumpSrv;
using System.Runtime.InteropServices;

using var cancellation = new CancellationTokenSource();
Console.CancelKeyPress += (_, e) =>
{
    e.Cancel = true;
    cancellation.Cancel();
};
using var terminate = OperatingSystem.IsWindows() ? null :
    PosixSignalRegistration.Create(PosixSignal.SIGTERM, context =>
    {
        context.Cancel = true;
        cancellation.Cancel();
    });
using var hangup = OperatingSystem.IsWindows() ? null :
    PosixSignalRegistration.Create(PosixSignal.SIGHUP, context =>
    {
        context.Cancel = true;
        cancellation.Cancel();
    });
return await CommandLine.ExecuteAsync(args, cancellation.Token);
