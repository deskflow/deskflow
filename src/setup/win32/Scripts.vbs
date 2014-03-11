Function RestartExplorerPrompt
    
    Dim message
    message = "The Windows Explorer process needs to be restarted. " & _
        "A reboot is required if you do not wish to do this. " & vbCr & vbCr & _
        "Would you like setup to restart the Windows Explorer process?"

    answer = MsgBox(message, vbSystemModal Or vbYesNo Or vbQuestion, "Restart Explorer")
    
    If answer = vbYes Then
        restart = "yes"
    Else
        restart = "no"
    End If

    Session.Property("RESTART_EXPLORER") = restart

End Function

Function RestartExplorer
    
    Set wmi = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\cimv2")
    Set processList = wmi.ExecQuery("Select * from Win32_Process Where Name = 'explorer.exe'")

    For Each process in processList
        process.Terminate(1)
    Next

    Set shell = CreateObject("Wscript.Shell") 
    shell.Run "explorer.exe"

End Function
