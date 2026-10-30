import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}
// Signing material stays outside the checkout. No credentials in Gradle arguments.
val signingFile = System.getenv("SCREENBUDDY_SIGNING_PROPERTIES")?.let { file(it) }
val releaseSigning = Properties().apply {
    if (signingFile != null) signingFile.inputStream().use { load(it) }
}
if (signingFile != null) {
    require(listOf("storeFile", "storePassword", "keyAlias", "keyPassword").all {
        !releaseSigning.getProperty(it).isNullOrBlank()
    }) { "Incomplete Companion signing properties" }
}
android {
    namespace = "de.fuchslabs.display"
    compileSdk = 34
    defaultConfig {
        applicationId = "de.fuchslabs.display"
        minSdk = 26
        targetSdk = 34
        versionCode = 4
        versionName = "1.0.0"
    }
    signingConfigs {
        if (signingFile != null) create("companionRelease") {
            storeFile = file(releaseSigning.getProperty("storeFile"))
            storePassword = releaseSigning.getProperty("storePassword")
            keyAlias = releaseSigning.getProperty("keyAlias")
            keyPassword = releaseSigning.getProperty("keyPassword")
        }
    }
    buildTypes { release {
        isMinifyEnabled = false
        if (signingFile != null) signingConfig = signingConfigs.getByName("companionRelease")
    } }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    buildFeatures { viewBinding = false }
}
dependencies {
    implementation(project(":protocol"))
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
    // ZXing-Scanner ohne Google Play Services (Kamera + ZXing-Decoder lokal).
    implementation("com.journeyapps:zxing-android-embedded:4.3.0")
}
