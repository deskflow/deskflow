# Barrier

Eliminate the barrier between your machines. 
Find [releases here](https://github.com/debauchee/barrier/releases).

### Contact info:

- `#barrier` on freenode

#### CI Build Status

Master branch overall build status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)
* Linux Build Status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Linux%20Build)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)
* Mac Build Status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Mac%20Build)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)
* Windows Debug Build Status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Windows%20Build&configuration=Windows%20Build%20Debug)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)
* Windows Release Build Status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Windows%20Build&configuration=Windows%20Build%20Release%20with%20Release%20Installer)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)

Our CI Builds are provided by Microsoft Azure Pipelines.

### What is it?

Barrier is KVM software forked from Symless's synergy 1.9 codebase. Synergy was a commercialized reimplementation of the original CosmoSynergy written by Chris Schoeneman.

### What's different?

Whereas synergy has moved beyond its goals from the 1.x era, Barrier aims to maintain that simplicity. Barrier will let you use your keyboard and mouse from machine A to control machine B (or more). It's that simple.

### Project goals

Hassle-free reliability. We are users, too. Barrier was created so that we could solve the issues we had with synergy and then share these fixes with other users.

Compatibility. We use more than one operating system and you probably do, too. Windows, OSX, Linux, FreeBSD... Barrier should "just work". We will also have our eye on Wayland when the time comes.

Communication. Everything we do is in the open. Our issue tracker will let you see if others are having the same problem you're having and will allow you to add additional information. You will also be able to see when progress is made and how the issue gets resolved.

### Contact & support

Please be aware that the *only* way to draw our attention to a bug is to create a new PR in the issue tracker. Write a clear, concise, detailed report and you will get a clear, concise, detailed response. Priority is always given to issues that affect a wider range of users.

For short and simple questions or to just say hello find us on the Freenode IRC network in the #barrier channel.

### Contributions

At this time we are looking for developers to help fix the issues found in the issue tracker. Submit pull requests once you've polished up your patch and we'll review and possibly merge it.

### FAQ

Q: Does drag and drop work on linux?

A: No

Q: What OSes are supported?

A:
  - Windows 7, 8, 8.1, and 10
  - MacOS/OS X
  - Linux
  - FreeBSD

Q: Are 32-bit versions of Windows supported?

A: No
