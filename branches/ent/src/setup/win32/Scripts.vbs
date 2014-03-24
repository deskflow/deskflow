Function RestartExplorerPrompt
    
    Dim message
    message = "The Windows Explorer process needs to be restarted. " & _
        vbCr & vbCr & "Would you like setup to do this?"

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
