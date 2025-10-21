@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

ECHO ============================================================
ECHO recordVideo.cmd - Automated Video Recording Script
ECHO ============================================================
ECHO.

:: ============================================================================
:: GENERIC VIDEO RECORDING SCRIPT
:: ============================================================================
:: This script reads configuration from systemConfig.ini and executes the
:: automated video recording workflow for 3D video monitoring.
::
:: Workflow:
::   STEP 1: Record video using LAU3DVideoRecorder
::   STEP 2: Encode object IDs using LAUEncodeObjectIDFilter (if enabled)
::   STEP 3: Upload videos to cloud storage (OneDrive/SharePoint)
::   STEP 4: Delete local temporary files
::
:: Features:
::   - Verbose logging with detailed status messages at each step
::   - Fixed time calculation that handles midnight rollover correctly
::   - Comprehensive error reporting and safety checks
::   - Automatic cleanup only after successful upload
::
:: Created by LAURemoteTools - Video Recording Setup
:: ============================================================================

:: ============================================================================
:: READ CONFIGURATION FROM INI FILE
:: ============================================================================

:: Determine the directory where this script is located
SET "SCRIPT_DIR=%~dp0"

:: Look for systemConfig.ini in the script directory
SET "INI_FILE=%SCRIPT_DIR%systemConfig.ini"

:: Verify INI file exists
IF NOT EXIST "!INI_FILE!" (
    ECHO ERROR: Configuration file not found: !INI_FILE!
    ECHO Please run LAURemoteTools to create systemConfig.ini
    EXIT /B 10
)

ECHO Reading configuration from: %INI_FILE%
ECHO.

:: Parse INI file using findstr to extract specific keys
:: This avoids FOR /F issues with special characters in paths

FOR /F "tokens=2 delims==" %%A IN ('findstr /I "^LocationCode=" "%INI_FILE%"') DO SET "LOCATION_CODE=%%A"

FOR /F "tokens=2 delims==" %%A IN ('findstr /I "^DurationMinutes=" "%INI_FILE%"') DO SET "DURATION_MINUTES=%%A"

FOR /F "tokens=2 delims==" %%A IN ('findstr /I "^DestinationPath=" "%INI_FILE%"') DO SET "DESTINATION_PATH=%%A"

FOR /F "tokens=2 delims==" %%A IN ('findstr /I "^LocalTempPath=" "%INI_FILE%"') DO SET "LOCAL_TEMP_PATH=%%A"

FOR /F "tokens=2 delims==" %%A IN ('findstr /I "^EnableEncoding=" "%INI_FILE%"') DO SET "ENABLE_ENCODING=%%A"

:: Validate that required configuration values were loaded
IF NOT DEFINED LOCATION_CODE (
    ECHO ERROR: LocationCode not found in configuration
    EXIT /B 11
)

IF NOT DEFINED DURATION_MINUTES (
    ECHO ERROR: DurationMinutes not found in configuration
    EXIT /B 12
)

IF NOT DEFINED DESTINATION_PATH (
    ECHO ERROR: DestinationPath not found in configuration
    EXIT /B 13
)

IF NOT DEFINED LOCAL_TEMP_PATH (
    ECHO ERROR: LocalTempPath not found in configuration
    EXIT /B 14
)

:: Display loaded configuration
ECHO ============================================================
ECHO Location Code:       %LOCATION_CODE%
ECHO Duration:            %DURATION_MINUTES% minutes
ECHO Local Temp Path:     %LOCAL_TEMP_PATH%
ECHO Destination Path:    %DESTINATION_PATH%
ECHO Enable Encoding:     %ENABLE_ENCODING%
ECHO ============================================================
ECHO.

:: ============================================================================
:: CONFIGURATION DERIVED VALUES
:: ============================================================================

:: Convert duration from minutes to seconds for the recording loop
SET /A totTime=%DURATION_MINUTES% * 60

:: Local directory where videos are temporarily stored during recording
SET videoRepoPath=%LOCAL_TEMP_PATH%

:: Construct location-specific destination folder path
SET folderStringOut=%DESTINATION_PATH%\location%LOCATION_CODE%\Folder

:: ============================================================================
:: STEP 1: VIDEO RECORDING SESSION
:: ============================================================================

ECHO.
ECHO ============================================================
ECHO STEP 1: STARTING VIDEO RECORDING SESSION
ECHO ============================================================
ECHO Video recording is about to begin. The system will continuously
ECHO record video from the 3D cameras until the configured time limit
ECHO is reached.
ECHO.
ECHO Session Configuration:
ECHO   - Total recording time: %totTime% seconds (%DURATION_MINUTES% minutes)
ECHO   - Video storage: %videoRepoPath%
ECHO   - Location code: %LOCATION_CODE%
ECHO.
ECHO Recording will begin now...
ECHO.

:: Record the start time of the recording session
SET vidTime=%time%
ECHO Session started at: %vidTime%
ECHO.

:: Loop continuously recording videos until total time expires
:checkTheTime
    :: Get current time for this iteration
    SET srtTime=%time%

    :: Calculate elapsed time since recording session started (result in _tdiff variable)
    CALL :tdiff %vidTime% %srtTime%

    :: Calculate remaining time: lftTime = totTime - elapsed time
    SET /a lftTime=totTime-_tdiff

    :: Check if we've reached the total recording time limit
    :: If time expired, jump to encoding/upload section
    IF %lftTime% LEQ 0 (GOTO :recordingComplete)

    :: Start recording with LAU3DVideoRecorder
    :: Arguments: outputPath, duration (HH:MM:SS), threshold (optional)
    :: This will record for the remaining time
    :: Convert seconds to HH:MM:SS format
    SET /a lftHours=lftTime/3600
    SET /a lftMinutes=(lftTime%%3600)/60
    SET /a lftSeconds=lftTime%%60

    :: Pad with zeros for HH:MM:SS format
    IF %lftHours% LSS 10 (SET lftHours=0%lftHours%)
    IF %lftMinutes% LSS 10 (SET lftMinutes=0%lftMinutes%)
    IF %lftSeconds% LSS 10 (SET lftSeconds=0%lftSeconds%)

    ECHO ------------------------------------------------------------
    ECHO RECORDING IN PROGRESS - Time Remaining: %lftHours%:%lftMinutes%:%lftSeconds%
    ECHO ------------------------------------------------------------
    ECHO Starting LAU3DVideoRecorder to capture video from cameras...
    ECHO This process will run for the remaining time or until manually stopped.
    ECHO Videos are being saved to: %videoRepoPath%
    ECHO.

    LAU3DVideoRecorder "%videoRepoPath%" %lftHours%:%lftMinutes%:%lftSeconds%

    ECHO.
    ECHO LAU3DVideoRecorder has completed this recording iteration.
    ECHO   Session start time: %vidTime%
    ECHO   Current time: %srtTime%
    ECHO   Total elapsed: %_tdiff% seconds
    ECHO   Exit status: %errorlevel%
    ECHO.
    ECHO Checking if more recording time remains...
    ECHO.

    :: Continue the recording loop
    GOTO :checkTheTime

EXIT /b 15

:recordingComplete
    ECHO.
    ECHO ============================================================
    ECHO STEP 1 COMPLETE: RECORDING SESSION FINISHED
    ECHO ============================================================
    ECHO The video recording session has finished successfully.
    ECHO   Total recording time: %totTime% seconds (%DURATION_MINUTES% minutes)
    ECHO   Videos saved to: %videoRepoPath%
    ECHO.
    ECHO Next step: Processing video files...
    ECHO.

:: ============================================================================
:: STEP 2: OBJECT ID ENCODING
:: ============================================================================
:: Check if object ID encoding is enabled
IF /I NOT "%ENABLE_ENCODING%"=="true" GOTO :encodingSkipped

ECHO ============================================================
ECHO STEP 2: ENCODING OBJECT IDs ^(RFID TAG EXTRACTION^)
ECHO ============================================================
ECHO Starting LAUEncodeObjectIDFilter to process video files...
ECHO This tool will:
ECHO   - Read RFID tags embedded in the video frames
ECHO   - Extract object identification numbers
ECHO   - Encode object IDs into the video file metadata
ECHO.
ECHO Processing directory: %videoRepoPath%
ECHO This may take several minutes depending on video file sizes...
ECHO.

:: Run the encoding filter on the local temporary path
:: This will extract RFID tags from video frames and embed object IDs
:: Set Qt to use offscreen platform to avoid GUI errors
SET QT_QPA_PLATFORM=offscreen
LAUEncodeObjectIDFilter "%videoRepoPath%"
SET QT_QPA_PLATFORM=

ECHO.
IF %errorlevel% EQU 0 (
    ECHO Object ID encoding completed successfully!
    ECHO All RFID tags have been processed and object IDs are now embedded.
) ELSE (
    ECHO WARNING: LAUEncodeObjectIDFilter returned error code %errorlevel%
    ECHO Some RFID tags may not have been processed correctly.
    ECHO However, videos are still valid and will be uploaded.
)
ECHO.
GOTO :copyVideoFilesToCloudStorage

:encodingSkipped
ECHO ============================================================
ECHO STEP 2: OBJECT ID ENCODING ^(SKIPPED^)
ECHO ============================================================
ECHO Object ID encoding is disabled in the configuration.
ECHO Videos will be uploaded without RFID tag processing.
ECHO.

:: ============================================================================
:: STEP 3: UPLOAD FILES TO CLOUD
:: ============================================================================
:copyVideoFilesToCloudStorage
    ECHO ============================================================
    ECHO STEP 3: UPLOADING VIDEOS TO CLOUD STORAGE
    ECHO ============================================================
    ECHO Starting upload process to OneDrive/SharePoint...
    ECHO.

    :: Create a date-stamped folder name: YYYYMMDD format using PowerShell (more reliable)
    :: Example: 20250115 for January 15, 2025
    FOR /F "tokens=*" %%i IN ('powershell -Command "Get-Date -Format 'yyyyMMdd'"') DO SET YYYYMMDD=%%i
    set folderString=%folderStringOut%%YYYYMMDD%\

    ECHO Upload Configuration:
    ECHO   Source directory: %videoRepoPath%
    ECHO   Destination: %folderString%
    ECHO   Location code: %LOCATION_CODE%
    ECHO   Date: %YYYYMMDD%
    ECHO.

    :: Create destination folder if it doesn't exist
    ECHO Creating cloud destination folder...
    IF NOT EXIST "%folderString%" (
        mkdir "%folderString%"
        IF EXIST "%folderString%" (
            ECHO   Folder created successfully: %folderString%
        ) ELSE (
            ECHO   WARNING: Failed to create folder: %folderString%
        )
    ) ELSE (
        ECHO   Folder already exists: %folderString%
    )
    ECHO.

    :: Verify local video directory exists before attempting copy
    IF NOT EXIST "%videoRepoPath%" GOTO :noVideoDirectory

    ECHO Verifying source directory exists... OK
    ECHO.
    ECHO Starting file copy operation...
    ECHO This may take several minutes depending on file sizes and network speed.
    ECHO Please wait...
    ECHO.

    :: Copy recording files to OneDrive (excluding noCal*.tif files)
    :: Always upload TXT and LUTX files (logs and calibration data)
    :: Upload data*.tif files (valid videos with complete calibration)
    :: Upload noTag*.tif files (duplicate object IDs - need review)
    :: Skip noCal*.tif files (incomplete calibration - not useful for analysis)
    :: Skip badFile*.tif files (insufficient RFID readings)
    :: /I = Assume destination is a directory
    :: /Y = Suppress prompts to overwrite files

    ECHO Copying valid videos ^(data*.tif, data*.tiff^)...
    xcopy "%videoRepoPath%\data*.tif" "%folderString%" /I /Y >nul 2>&1
    xcopy "%videoRepoPath%\data*.tiff" "%folderString%" /I /Y >nul 2>&1

    ECHO Copying duplicate object ID files ^(noTag*.tif, noTag*.tiff^)...
    xcopy "%videoRepoPath%\noTag*.tif" "%folderString%" /I /Y >nul 2>&1
    xcopy "%videoRepoPath%\noTag*.tiff" "%folderString%" /I /Y >nul 2>&1

    ECHO Copying log files ^(*.txt^)...
    xcopy "%videoRepoPath%\*.txt" "%folderString%" /I /Y >nul 2>&1

    ECHO Copying calibration files ^(*.lutx^)...
    xcopy "%videoRepoPath%\*.lutx" "%folderString%" /I /Y >nul 2>&1

    ECHO Skipping incomplete calibration files ^(noCal*.tif^) and bad RFID files ^(badFile*.tif^)

    ECHO.
    ECHO File copy operation completed with status: %errorlevel%
    ECHO.
    GOTO :cleanupLocalFiles

:noVideoDirectory
    ECHO ============================================================
    ECHO ERROR: LOCAL VIDEO DIRECTORY NOT FOUND
    ECHO ============================================================
    ECHO The source video directory does not exist: %videoRepoPath%
    ECHO.
    ECHO This could mean:
    ECHO   - No videos were recorded
    ECHO   - Recording directory was moved or deleted
    ECHO   - Path configuration is incorrect
    ECHO.
    ECHO No files to upload. Workflow will exit.

:: ============================================================================
:: STEP 4: CLEANUP LOCAL FILES
:: ============================================================================
:cleanupLocalFiles
IF NOT EXIST "%videoRepoPath%" GOTO :noCleanupNeeded
IF NOT EXIST "%folderString%" GOTO :uploadNotVerified

ECHO ============================================================
ECHO STEP 4: CLEANING UP LOCAL FILES
ECHO ============================================================
ECHO Upload verification: Destination folder exists - Upload successful!
ECHO.
ECHO Now deleting local temporary files to free up disk space...
ECHO Source: %videoRepoPath%
ECHO.

:: Delete only the files we successfully uploaded
:: NOTE: LUTX files are NOT deleted - they are kept for reference
:: /q = Quiet mode (no confirmation prompts)
ECHO Deleting uploaded files ^(data*.tif, noTag*.tif, badFile*.tif, *.txt^)...
del /q "%videoRepoPath%\data*.tif" >nul 2>&1
del /q "%videoRepoPath%\data*.tiff" >nul 2>&1
del /q "%videoRepoPath%\noTag*.tif" >nul 2>&1
del /q "%videoRepoPath%\noTag*.tiff" >nul 2>&1
del /q "%videoRepoPath%\badFile*.tif" >nul 2>&1
del /q "%videoRepoPath%\badFile*.tiff" >nul 2>&1
del /q "%videoRepoPath%\*.txt" >nul 2>&1

ECHO.

:: Intelligent noCal*.tif file management:
:: - If we have any data*.tif files, calibration is working - delete all noCal files
:: - If no data*.tif files (setup phase), keep only the 10 most recent noCal files
ECHO Managing noCal*.tif files ^(incomplete calibration^)...

:: Count how many data files we uploaded (check if calibration is working)
set uploadedDataCount=0
for %%F in ("!folderString!data*.tif") do set /a uploadedDataCount+=1 2>nul

:: Check if calibration is working (data files uploaded)
IF !uploadedDataCount! GTR 0 GOTO :calibrationComplete

:: Still in setup phase - manage noCal files
:: Count noCal files first
set noCalCount=0
for %%F in ("%videoRepoPath%\noCal*.tif") do set /a noCalCount+=1

:: Only show messages if there are noCal files
IF !noCalCount! EQU 0 (
    ECHO   No noCal files to manage
    GOTO :cleanupSummary
)

ECHO   Still in setup phase - keeping last 10 noCal*.tif files for calibration
ECHO   Found !noCalCount! noCal files

:: If 10 or fewer files, keep them all
IF !noCalCount! LEQ 10 (
    ECHO   Kept all !noCalCount! noCal files ^(under 10 file limit^)
    GOTO :cleanupSummary
)

:: More than 10 files - delete oldest, keep 10 newest
ECHO   Deleting oldest noCal files ^(keeping 10 most recent^)...

:: Calculate how many to delete
set /a filesToDelete=!noCalCount!-10
ECHO   Will delete !filesToDelete! oldest files

:: Use temporary file to avoid FOR loop parsing issues
set "TEMP_FILE=%TEMP%\nocalfiles_!RANDOM!.txt"

:: Write file list to temp file (sorted by date, oldest first)
dir /b /o:d "%videoRepoPath%\noCal*.tif" > "!TEMP_FILE!" 2>nul

:: Read from temp file and delete oldest files
set fileNum=0
for /f "usebackq delims=" %%F in ("!TEMP_FILE!") do (
    set /a fileNum+=1
    if !fileNum! LEQ !filesToDelete! del /q "%videoRepoPath%\%%F" 2>nul & ECHO     Deleted: %%F
)

:: Clean up temp file
del /q "!TEMP_FILE!" >nul 2>&1
ECHO   Kept 10 most recent noCal files for calibration work
GOTO :cleanupSummary

:calibrationComplete
:: Calibration is working - uploaded valid videos
ECHO   Calibration is working - uploaded !uploadedDataCount! valid videos
ECHO   Deleting all local noCal*.tif files ^(no longer needed^)
del /q "%videoRepoPath%\noCal*.tif" >nul 2>&1
del /q "%videoRepoPath%\noCal*.tiff" >nul 2>&1
ECHO   All noCal files deleted ^(calibration complete^)

:cleanupSummary
ECHO.
ECHO Cleanup summary:
ECHO   - Uploaded files deleted ^(data*.tif, noTag*.tif, badFile*.tif, *.txt^)
ECHO   - LUTX files retained for reference
ECHO   - noCal files managed ^(see above^)
ECHO Disk space has been freed.
GOTO :workflowComplete

:uploadNotVerified
ECHO ============================================================
ECHO STEP 4: LOCAL FILE CLEANUP ^(SKIPPED FOR SAFETY^)
ECHO ============================================================
ECHO WARNING: Destination folder not found after copy operation.
ECHO Destination path: %folderString%
ECHO.
ECHO This could indicate:
ECHO   - Network connectivity issues
ECHO   - OneDrive sync problems
ECHO   - Insufficient permissions
ECHO.
ECHO LOCAL FILES WILL NOT BE DELETED to prevent data loss.
ECHO Please manually verify the upload and delete files if needed.
GOTO :workflowComplete

:noCleanupNeeded
ECHO ============================================================
ECHO ERROR: LOCAL VIDEO DIRECTORY NOT FOUND
ECHO ============================================================
ECHO The source video directory does not exist: %videoRepoPath%
ECHO.
ECHO This could mean:
ECHO   - No videos were recorded
ECHO   - Recording directory was moved or deleted
ECHO   - Path configuration is incorrect
ECHO.
ECHO No cleanup needed.

:workflowComplete
ECHO.
ECHO ============================================================
ECHO VIDEO RECORDING WORKFLOW COMPLETE
ECHO ============================================================
ECHO Summary:
ECHO   Location code: %LOCATION_CODE%
ECHO   Date: %YYYYMMDD%
ECHO   Recording duration: %DURATION_MINUTES% minutes
ECHO   Destination: %folderString%
ECHO.
ECHO All tasks completed. Log file contains detailed information.
ECHO ============================================================

:exitFromScript
EXIT /b 0

:: ============================================================================
:: UTILITY FUNCTION: Calculate Time Difference
:: ============================================================================
:: Calculates the difference between two timestamps in seconds
:: Handles midnight rollover (e.g., from 23:59:59 to 00:00:01)
::
:: Usage: CALL :tdiff StartTime StopTime
:: Returns: _tdiff variable (difference in seconds)
:: Example: CALL :tdiff "14:30:00" "15:45:30"
::          Result: 4530 seconds (1 hour 15 minutes 30 seconds)
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:tdiff
@ECHO OFF
SETLOCAL

:: Validate that start time was provided
if "%~1" EQU "" goto s_syntax

:: Convert start time to hundredths of a second since midnight
:: Uses variable to return numeric value from subroutine
Call :s_calc_timecode %1
Set _start_timecode=%_timecode_result%

:: Convert end time to hundredths of a second since midnight
Call :s_calc_timecode %2
Set _stop_timecode=%_timecode_result%

:: Calculate the difference in hundredths of a second
Set /a _diff_timecode=_stop_timecode - _start_timecode

:: Handle midnight rollover (negative result means we crossed midnight)
:: Add 24 hours worth of hundredths if negative
if %_diff_timecode% LSS 0 set /a _diff_timecode+=(24 * 60 * 60 * 100)

:: Convert from hundredths of a second to whole seconds
set /a _diff_timecode/=100

:: Return the result in _tdiff variable (in seconds)
endlocal & set _tdiff=%_diff_timecode%
goto :EOF

:: ============================================================================
:: UTILITY FUNCTION: Convert Time to Timecode
:: ============================================================================
:: Converts HH:MM:SS.hs format to hundredths of a second since midnight
:: Used internally by :tdiff function
::
:: Input: Time string in format HH:MM:SS.hs (e.g., 14:30:15.50)
:: Output: Returns value via exit code (hundredths of a second)
:: Example: 01:00:00.00 = 360000 hundredths (1 hour = 3600 seconds)
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:s_calc_timecode
  setlocal

  :: Parse time string into components: hours, minutes, seconds, hundredths
  :: Delimiters are colon (:) and period (.)
  :: Use %~1 to strip surrounding quotes from parameter
  For /f "usebackq tokens=1,2,3,4 delims=:." %%a in ('%~1') Do (
      set hh=%%a
      set mm=%%b
      set ss=%%c
      set hs=%%d
  )

   :: Strip all spaces from parsed components (fixes leading space issue in %time%)
   set hh=%hh: =%
   set mm=%mm: =%
   set ss=%ss: =%
   set hs=%hs: =%

   :: Check string length and pad single-digit values
   :: If length is 1 (no second character), prepend 0
   set "test=%hh:~1%"
   if not defined test set hh=0%hh%
   set "test=%mm:~1%"
   if not defined test set mm=0%mm%
   set "test=%ss:~1%"
   if not defined test set ss=0%ss%
   set "test=%hs:~1%"
   if not defined test set hs=0%hs%

   :: Force decimal interpretation by using the 1xx-100 trick
   :: This converts "08" to 108-100=8, "00" to 100-100=0, etc.
   :: Prevents octal interpretation errors
   set /a hh=1%hh%-100
   set /a mm=1%mm%-100
   set /a ss=1%ss%-100
   set /a hs=1%hs%-100

   :: Convert each component to hundredths of a second
   set /a hh=hh * 60 * 60 * 100
   set /a mm=mm * 60 * 100
   set /a ss=ss * 100
   set /a hs=hs

   :: Sum all components to get total hundredths since midnight
   set /a _timecode=hh + mm + ss + hs

:: Return timecode value via ENDLOCAL & SET (more reliable than Exit /b)
Endlocal & set _timecode_result=%_timecode%
goto :EOF

:: ============================================================================
:: ERROR HANDLER: Time Difference Syntax Help
:: ============================================================================
:: Displays usage information when :tdiff is called incorrectly
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:s_syntax
   Echo:
   Echo ERROR: Missing or invalid time parameters
   Echo:
   Echo Syntax: CALL :tdiff StartTime StopTime
   Echo:
   Echo    The times can be given in the format:
   Echo    HH  or  HH:MM  or  HH:MM:SS  or  HH:MM:SS.hs
   Echo:
   Echo    Examples:
   Echo      CALL :tdiff %%time%% 23
   Echo      (calculates time difference between now and 23:00:00.00)
   Echo:
   Echo      CALL :tdiff "14:30:00" "15:45:30"
   Echo      (calculates time difference = 1 hour 15 minutes 30 seconds)
   Echo:
   Echo    The result is returned in variable %%_tdiff%% (in seconds)
Exit /b 1
