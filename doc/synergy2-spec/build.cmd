@set name=synergy2-spec

:: TODO: loop for all .mp files
@mpost uml\server.mp --output-directory=uml

@pdflatex %name%
@if not %ERRORLEVEL% equ 0 goto abort

:: run twice to resolve dependencies
@if exist %name%.pdf pdflatex %name%
@if not %ERRORLEVEL% equ 0 goto abort

:: then run the file if we actually generated stuff
@if exist %name%.pdf %name%.pdf

goto end

:abort
@echo
@echo Aborting with error code: %ERRORLEVEL%

:end
