param([Parameter(ValueFromRemainingArguments = $true)][string[]]$GradleArguments)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$java = Join-Path $repo '.toolchain\jdk-17'
$sdk = Join-Path $repo '.toolchain\android-sdk'
if (-not (Test-Path (Join-Path $java 'bin\java.exe'))) { throw "JDK 17 not found: $java" }
if (-not (Test-Path (Join-Path $sdk 'platforms\android-29'))) { throw "Android SDK platform 29 not installed: $sdk" }
if (-not (Test-Path (Join-Path $sdk 'ndk\23.2.8568313'))) { throw "Android NDK 23.2.8568313 not installed: $sdk" }
if (-not $GradleArguments) { $GradleArguments = @(':app:assembleDebug') }
$oldJava = $env:JAVA_HOME
$oldAndroid = $env:ANDROID_HOME
$oldSdk = $env:ANDROID_SDK_ROOT
try {
    $env:JAVA_HOME = $java
    $env:ANDROID_HOME = $sdk
    $env:ANDROID_SDK_ROOT = $sdk
    Push-Location $repo
    try {
        # Native tools may write progress and warnings to stderr even on success.
        $ErrorActionPreference = 'Continue'
        & (Join-Path $java 'bin\java.exe') '-classpath' 'gradle\wrapper\gradle-wrapper.jar' 'org.gradle.wrapper.GradleWrapperMain' @GradleArguments
        if ($LASTEXITCODE -ne 0) { throw "Gradle failed with exit code $LASTEXITCODE" }
    } finally { Pop-Location }
} finally {
    $env:JAVA_HOME = $oldJava
    $env:ANDROID_HOME = $oldAndroid
    $env:ANDROID_SDK_ROOT = $oldSdk
}
