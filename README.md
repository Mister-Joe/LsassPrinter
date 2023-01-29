# LsassPrinter
How to dump lsass.exe via spoolsv.exe and DLL side-loading.
# Overview
Generally spoolsv.exe will load ``C:\Windows\System32\WSDPrintProxy.DLL`` and/or ``C:\Windows\System32\spool\prtprocs\x64\winprint.dll`` everytime the computer boots. By leveraging dll-side loading and replacing ``WSDPrintProxy.DLL`` or ``winprint.dll`` with a dll of our own creation, we can gain privileged code execution in the spoolsv process. Of course, you'll need some sort of administrative rights to mess with DLLs in System32. To my surprise, this is an effective way to dump lsass without being detected by either Windows Defender or CrowdStrike Falcon (for now).

> If you've configured a printer through the Windows "add a printer" functionality, generally spoolsv will load ``WSDPrintProxy.DLL`` everytime the computer boots. On some computers spoolsv doesn't appear to load ``WSDPrintProxy.DLL`` at all though, and I'm not sure why. In those cases, you may have better luck with ``winprint.dll``.

Credits to: [ired.team](https://www.ired.team) for their code on MiniDumpWriteDump and minidumpCallback.
# The DLL
Apologies in advance for my spaghetti code. 

When the case ``DLL_PROCESS_ATTACH`` is triggered, we call the ``dump`` function. The dump function does some setup of structures, finds the PID of lsass, obtains a handle to lsass with process access rights ``PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE``, and then calls ``MiniDumpWriteDump``.

Instead of passing a handle to an output file as an argument to ``MiniDumpWriteDump``, we instead pass a pointer to a ``MINIDUMP_CALLBACK_INFORMATION`` structure that contains the member ``CallbackRoutine`` which contains a pointer to our callback function ``minidumpCallback``.

This callback is necessary because we need to save the output of ``MiniDumpWriteDump`` to memory and XOR it before writing it to disk. AVs & EDRs will actually detect lsass dump files based on signatures if not obfuscated in some way, even if the initial dump of lsass went undetected.
# POC
``WSDPrintProxy.DLL`` is owned by ``NT SERVICE\TrustedInstaller`` by default, so we'll need to take ownership first before we can make changes to it.

![Capture1](https://user-images.githubusercontent.com/16895391/215357267-c994d8a8-f361-4666-833b-0679d21c153b.PNG)

Assuming we're a local administrator, we can edit the ACLs to give the administrators group full access.

![Capture2](https://user-images.githubusercontent.com/16895391/215357337-9c36be08-4ca7-4158-9d44-326aa9ccdbde.PNG)

We'll probably want to make a copy of the legitmate ``WSDPrintProxy.DLL`` so we can restore it later after dumping lsass.

![Capture3](https://user-images.githubusercontent.com/16895391/215357411-045c314c-5993-4d6e-887b-a0834c7a01d8.PNG)

Now we'll move our malicious version of ``WSDPrintProxy.DLL`` into place.

![Capture4](https://user-images.githubusercontent.com/16895391/215357445-6fbb47a3-0d3a-4db2-b85f-96f5ac80202a.PNG)

Now we can either wait for the system to be shutdown/rebooted, or we can force a reboot manually.

![Capture5](https://user-images.githubusercontent.com/16895391/215357480-3006f14f-7c61-4c3f-8471-9cc4fe7f1841.PNG)

After rebooting, we find our XOR'd dump file written to C:\.

![Capture6](https://user-images.githubusercontent.com/16895391/215357543-aa1c398e-dc9d-423f-944d-3e983c8d8e82.PNG)

At this point, we can transfer the dump file to a Linux machine under our control and use the little ``xor_decrypt.py`` script I made to decrypt the dump file, then we can use ``pypykatz`` to parse the dump file.

![Capture7](https://user-images.githubusercontent.com/16895391/215357588-71ea88bf-4573-40aa-aad4-1310933a6949.PNG)

To my surprise, both Windows Defender and CrowdStrike Falcon did not detect this.

<img src="https://user-images.githubusercontent.com/16895391/215357655-940dd8cb-3489-4e06-9c75-dce4ea7e2f8a.PNG" width=25% height=25%/> <img src="https://user-images.githubusercontent.com/16895391/215357739-cce0564b-8516-4949-9233-ec13860d9c5d.png" width=50% height=50%/>

