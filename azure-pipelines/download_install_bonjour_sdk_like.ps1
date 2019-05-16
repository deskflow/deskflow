$ErrorActionPreference = "Stop"

New-Item -Force -ItemType Directory -Path ".\deps\"
$Wc = New-Object System.Net.WebClient
$Wc.DownloadFile('https://github.com/nelsonjchen/mDNSResponder/releases/download/v2019.05.08.1/x64_RelWithDebInfo.zip', 'deps\BonjourSDKLike.zip') ;
Write-Output 'Downloaded BonjourSDKLike Zip'
Write-Output 'Unzipping BonjourSDKLike Zip'
Remove-Item -Recurse -Force -ErrorAction Ignore .\deps\BonjourSDKLike
Expand-Archive .\deps\BonjourSDKLike.zip -DestinationPath .\deps\BonjourSDKLike
Write-Output 'Installed BonjourSDKLike Zip'
Remove-Item deps\BonjourSDKLike.zip
Write-Output 'Deleted BonjourSDKLike Zip'
